#include "VulkanDeviceImpl.hpp"
#include "VulkanPipelineImpl.hpp"

#include "VulkanBoilerplate/VulkanImage.hpp"

#include <cstddef>
#include <memory>
#include <ranges>
#include <stdexcept>
#include <vector>
#include <volk.h>

namespace QuakeLauncher::Renderer
{

namespace
{

[[nodiscard]] VkPrimitiveTopology ToVulkan(Quake::Renderer::PrimitiveTopology topology)
{
    switch (topology)
    {
    case Quake::Renderer::PrimitiveTopology::TriangleList:
        return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    case Quake::Renderer::PrimitiveTopology::TriangleStrip:
        return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP;
    case Quake::Renderer::PrimitiveTopology::LineList:
        return VK_PRIMITIVE_TOPOLOGY_LINE_LIST;
    }
    throw std::invalid_argument{"Unknown PrimitiveTopology"};
}

[[nodiscard]] VkCullModeFlags ToVulkan(Quake::Renderer::CullMode cullMode)
{
    switch (cullMode)
    {
    case Quake::Renderer::CullMode::None:
        return VK_CULL_MODE_NONE;
    case Quake::Renderer::CullMode::Front:
        return VK_CULL_MODE_FRONT_BIT;
    case Quake::Renderer::CullMode::Back:
        return VK_CULL_MODE_BACK_BIT;
    }
    throw std::invalid_argument{"Unknown CullMode"};
}

[[nodiscard]] VkFormat ToVulkan(Quake::Renderer::VertexFormat format)
{
    switch (format)
    {
    case Quake::Renderer::VertexFormat::Float:
        return VK_FORMAT_R32_SFLOAT;
    case Quake::Renderer::VertexFormat::Float2:
        return VK_FORMAT_R32G32_SFLOAT;
    case Quake::Renderer::VertexFormat::Float3:
        return VK_FORMAT_R32G32B32_SFLOAT;
    case Quake::Renderer::VertexFormat::Float4:
        return VK_FORMAT_R32G32B32A32_SFLOAT;
    }
    throw std::invalid_argument{"Unknown VertexFormat"};
}

} // namespace

VulkanDeviceImpl::VulkanDeviceImpl(const SDL::Context &context, const SDL::Window &window)
    : m_window{window},
      m_instance{context.GetVulkanInstanceExtensions()},
      m_surface{m_instance, window},
      m_device{m_instance, m_surface},
      m_allocator{m_instance, m_device},
      m_swapchain{m_device, m_surface},
      m_commandPool{VulkanCommandPoolBuilder{}.SetResettable().Build(m_device)},
      m_commandBuffers{m_commandPool.Allocate(kFramesInFlight)}
{
    for (std::uint32_t i{0}; i < kFramesInFlight; ++i)
    {
        m_imageAvailable.push_back(VulkanSemaphoreBuilder{}.Build(m_device));
        m_inFlight.push_back(VulkanFenceBuilder{}.SetSignaled().Build(m_device));
    }
}

VulkanDeviceImpl::~VulkanDeviceImpl()
{
    static_cast<void>(vkDeviceWaitIdle(m_device.GetLogicalDevice()));
}

Quake::Renderer::Pipeline &VulkanDeviceImpl::CreatePipeline(const Quake::Renderer::PipelineDescriptor &descriptor)
{
    const std::vector<std::uint32_t> vertexSpirv{
        m_shaderCompiler.Compile(ShaderStage::Vertex, descriptor.GetVertexShader())};
    const std::vector<std::uint32_t> fragmentSpirv{
        m_shaderCompiler.Compile(ShaderStage::Fragment, descriptor.GetFragmentShader())};

    const Quake::Renderer::VertexLayout &layout{descriptor.GetVertexLayout()};
    const auto attributes{layout.attributes |
                          std::views::transform(
                              [](const Quake::Renderer::VertexAttribute &attribute)
                              {
                                  return VkVertexInputAttributeDescription{attribute.location, 0,
                                                                           ToVulkan(attribute.format), attribute.offset};
                              }) |
                          std::ranges::to<std::vector>()};

    return *m_pipelines.emplace_back(std::make_unique<VulkanPipelineImpl>(
        VulkanGraphicsPipelineBuilder{}
            .SetVertexShader(vertexSpirv)
            .SetFragmentShader(fragmentSpirv)
            .SetTopology(ToVulkan(descriptor.GetTopology()))
            .SetCullMode(ToVulkan(descriptor.GetCullMode()))
            .SetColorFormat(m_swapchain.GetFormat())
            .SetSamples(ToVulkan(descriptor.GetSampleCount()))
            .SetVertexInput(layout.stride, attributes)
            .Build(m_device)));
}

Quake::Renderer::Texture &VulkanDeviceImpl::CreateTexture(const Quake::Renderer::TextureDescriptor &descriptor)
{
    const VkExtent2D extent{descriptor.MatchesBackbuffer() ? m_swapchain.GetExtent()
                                                           : VkExtent2D{descriptor.GetWidth(), descriptor.GetHeight()}};

    return *m_textures.emplace_back(std::make_unique<VulkanTextureImpl>(
        MakeImage(descriptor.GetFormat(), descriptor.GetSampleCount(), extent), descriptor.MatchesBackbuffer()));
}

Quake::Renderer::Buffer &VulkanDeviceImpl::CreateBuffer(const Quake::Renderer::BufferDescriptor &descriptor)
{
    VulkanBufferImpl &buffer{*m_buffers.emplace_back(std::make_unique<VulkanBufferImpl>(
        m_allocator, descriptor.GetSize(), descriptor.GetUsage(), m_frameIndex))};
    buffer.WriteAll(descriptor.GetInitialData());
    return buffer;
}

Quake::Renderer::Texture &VulkanDeviceImpl::GetBackbuffer()
{
    return *m_backbuffers.at(m_imageIndex);
}

Quake::Renderer::CommandList &VulkanDeviceImpl::BeginFrame()
{
    m_inFlight[m_frameIndex].Wait();

    m_frameActive = AcquireImage();
    if (!m_frameActive)
    {
        m_commandList.Skip();
        return m_commandList;
    }
    m_inFlight[m_frameIndex].Reset(); // Only reset once we know we will submit.

    m_commandList.Begin(m_commandBuffers[m_frameIndex], *m_backbuffers[m_imageIndex]);
    return m_commandList;
}

void VulkanDeviceImpl::EndFrame()
{
    if (!m_frameActive)
    {
        return;
    }
    m_frameActive = false;

    m_commandList.End();

    const VkQueue queue{m_device.GetGraphicsQueue()};
    VulkanSubmitBuilder{}
        .AddWait(m_imageAvailable[m_frameIndex], VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT)
        .AddCommandBuffer(m_commandList.GetCommandBuffer())
        .AddSignal(m_renderFinished[m_imageIndex], VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT)
        .Submit(queue, m_inFlight[m_frameIndex]);

    if (m_swapchain.Present(queue, m_imageIndex, m_renderFinished[m_imageIndex]) != SwapchainStatus::Optimal)
    {
        m_needsRecreate = true;
    }

    m_frameIndex = (m_frameIndex + 1) % kFramesInFlight;
}

bool VulkanDeviceImpl::AcquireImage()
{
    if (m_needsRecreate)
    {
        const VkExtent2D previousExtent{m_swapchain.GetExtent()};
        if (!m_swapchain.Recreate(m_device, m_surface, m_window))
        {
            return false; // Minimized.
        }

        m_renderFinished.clear();
        m_backbuffers.clear();
        for (std::size_t i{0}; i < m_swapchain.GetImages().size(); ++i)
        {
            m_renderFinished.push_back(VulkanSemaphoreBuilder{}.Build(m_device));
            m_backbuffers.push_back(
                std::make_unique<VulkanTextureImpl>(m_swapchain.GetImages()[i], m_swapchain.GetImageView(i),
                                                    m_swapchain.GetExtent(), m_swapchain.GetFormat()));
        }
        // Recreate() already waited for the device to go idle so we can just do this.
        const VkExtent2D currentExtent{m_swapchain.GetExtent()};
        if (currentExtent.width != previousExtent.width || currentExtent.height != previousExtent.height)
        {
            for (const auto &texture : m_textures)
            {
                if (texture->MatchesBackbuffer())
                {
                    texture->Replace(MakeImage(texture->GetFormat(), texture->GetSampleCount(), currentExtent));
                }
            }
        }
        m_needsRecreate = false;
    }

    const auto [status, imageIndex]{m_swapchain.AcquireNextImage(m_imageAvailable[m_frameIndex])};
    m_needsRecreate = status != SwapchainStatus::Optimal;
    m_imageIndex = imageIndex;
    return status != SwapchainStatus::OutOfDate;
}

VulkanImage VulkanDeviceImpl::MakeImage(Quake::Renderer::TextureFormat format, Quake::Renderer::SampleCount samples,
                                        VkExtent2D extent) const
{
    return VulkanImageBuilder{}
        .SetExtent(extent)
        .SetFormat(ToVulkan(format))
        .SetSamples(ToVulkan(samples))
        .SetUsage(VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT)
        .Build(m_device, m_allocator);
}

} // namespace QuakeLauncher::Renderer