#include "VulkanCommandListImpl.hpp"
#include "VulkanBufferImpl.hpp"
#include "VulkanPipelineImpl.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>
#include <stdexcept>
#include <volk.h>

namespace QuakeLauncher::Renderer
{

static_assert(kPushConstantSize == Quake::Renderer::kMaxPushConstantSize);

namespace
{

[[nodiscard]] const VulkanTextureImpl &Downcast(const Quake::Renderer::Texture &texture)
{
    return dynamic_cast<const VulkanTextureImpl &>(texture);
}

[[nodiscard]] VkAttachmentLoadOp ToVulkan(Quake::Renderer::LoadOp loadOp)
{
    switch (loadOp)
    {
    case Quake::Renderer::LoadOp::Load:
        return VK_ATTACHMENT_LOAD_OP_LOAD;
    case Quake::Renderer::LoadOp::Clear:
        return VK_ATTACHMENT_LOAD_OP_CLEAR;
    case Quake::Renderer::LoadOp::DontCare:
        return VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    }
    throw std::invalid_argument{"Unknown LoadOp"};
}

[[nodiscard]] VkAttachmentStoreOp ToVulkan(Quake::Renderer::StoreOp storeOp)
{
    switch (storeOp)
    {
    case Quake::Renderer::StoreOp::Store:
        return VK_ATTACHMENT_STORE_OP_STORE;
    case Quake::Renderer::StoreOp::DontCare:
        return VK_ATTACHMENT_STORE_OP_DONT_CARE;
    }
    throw std::invalid_argument{"Unknown StoreOp"};
}

/// For attachments whose old contents are irrelevant.
[[nodiscard]] VkImageMemoryBarrier2 MakeDiscardBarrier(VkImage image) noexcept
{
    return VulkanImageBarrierBuilder{}
        .SetImage(image)
        .SetLayouts(VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL)
        .SetSrc(VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT, VK_ACCESS_2_NONE)
        .SetDst(VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
                VK_ACCESS_2_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT)
        .Build();
}

} // namespace

void VulkanCommandListImpl::Begin(VulkanCommandBuffer commandBuffer, const VulkanTextureImpl &backbuffer)
{
    m_recording = true;
    m_commandBuffer = commandBuffer;
    m_backbuffer = &backbuffer;
    m_pipelineLayout = VK_NULL_HANDLE;

    const std::array toColorAttachment{MakeDiscardBarrier(backbuffer.GetImage())};

    m_commandBuffer.Begin(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
    m_commandBuffer.PipelineBarrier(toColorAttachment);
}

void VulkanCommandListImpl::End()
{
    const std::array toPresent{
        VulkanImageBarrierBuilder{}
            .SetImage(m_backbuffer->GetImage())
            .SetLayouts(VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR)
            .SetSrc(VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT, VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT)
            .SetDst(VK_PIPELINE_STAGE_2_NONE, VK_ACCESS_2_NONE)
            .Build()};

    m_commandBuffer.PipelineBarrier(toPresent);
    m_commandBuffer.End();
}

void VulkanCommandListImpl::BeginRenderPass(const Quake::Renderer::ColorAttachment &colorAttachment)
{
    if (!m_recording)
    {
        return;
    }

    const VulkanTextureImpl &target{colorAttachment.GetTarget() != nullptr ? Downcast(*colorAttachment.GetTarget())
                                                                           : *m_backbuffer};
    const VulkanTextureImpl *resolve{
        colorAttachment.GetResolveTarget() != nullptr ? &Downcast(*colorAttachment.GetResolveTarget()) : nullptr};

    // The backbuffer is already in the right layout (see Begin)
    std::array<VkImageMemoryBarrier2, 2> barriers{};
    std::size_t barrierCount{0};
    const auto discard{[&](const VulkanTextureImpl &texture)
                       {
                           if (texture.GetImage() != m_backbuffer->GetImage())
                           {
                               barriers[barrierCount++] = MakeDiscardBarrier(texture.GetImage());
                           }
                       }};
    if (colorAttachment.GetLoadOp() != Quake::Renderer::LoadOp::Load)
    {
        discard(target);
    }
    if (resolve != nullptr)
    {
        discard(*resolve);
    }
    if (barrierCount > 0)
    {
        m_commandBuffer.PipelineBarrier(std::span{barriers}.first(barrierCount));
    }

    const Quake::Renderer::Color &clearColor{colorAttachment.GetClearColor()};

    VulkanColorAttachmentBuilder attachmentBuilder;
    attachmentBuilder.SetImageView(target.GetImageView())
        .SetLoadOp(ToVulkan(colorAttachment.GetLoadOp()))
        .SetStoreOp(ToVulkan(colorAttachment.GetStoreOp()))
        .SetClearColor(clearColor.r, clearColor.g, clearColor.b, clearColor.a);
    if (resolve != nullptr)
    {
        attachmentBuilder.SetResolve(resolve->GetImageView());
    }
    const std::array attachments{attachmentBuilder.Build()};

    m_passImages = {target.GetImage(), resolve != nullptr ? resolve->GetImage() : VkImage{VK_NULL_HANDLE}};

    m_commandBuffer.BeginRendering(VkRect2D{{0, 0}, target.GetExtent()}, attachments);
    m_commandBuffer.SetViewportAndScissor(target.GetExtent());
}

void VulkanCommandListImpl::PushConstantsBytes(std::span<const std::byte> data)
{
    if (!m_recording)
    {
        return;
    }
    if (m_pipelineLayout == VK_NULL_HANDLE)
    {
        throw std::logic_error{"SetPipeline must be called before PushConstants"};
    }

    m_commandBuffer.PushConstants(m_pipelineLayout, kPushConstantStages, data);
}

void VulkanCommandListImpl::SetPipeline(const Quake::Renderer::Pipeline &pipeline)
{
    if (!m_recording)
    {
        return;
    }

    const auto &impl{dynamic_cast<const VulkanPipelineImpl &>(pipeline)};
    m_pipelineLayout = impl.GetPipelineLayout();
    m_commandBuffer.BindGraphicsPipeline(impl.GetPipeline());
}

void VulkanCommandListImpl::SetVertexBuffer(const Quake::Renderer::Buffer &buffer)
{
    if (!m_recording)
    {
        return;
    }

    m_commandBuffer.BindVertexBuffer(dynamic_cast<const VulkanBufferImpl &>(buffer).GetBuffer());
}

void VulkanCommandListImpl::SetIndexBuffer(const Quake::Renderer::Buffer &buffer, Quake::Renderer::IndexType indexType)
{
    if (!m_recording)
    {
        return;
    }

    m_commandBuffer.BindIndexBuffer(dynamic_cast<const VulkanBufferImpl &>(buffer).GetBuffer(), ToVulkan(indexType));
}

void VulkanCommandListImpl::DrawIndexed(std::uint32_t indexCount)
{
    if (!m_recording)
    {
        return;
    }

    m_commandBuffer.DrawIndexed(indexCount);
}

void VulkanCommandListImpl::Draw(std::uint32_t vertexCount)
{
    if (!m_recording)
    {
        return;
    }

    m_commandBuffer.Draw(vertexCount);
}

void VulkanCommandListImpl::EndRenderPass()
{
    if (!m_recording)
    {
        return;
    }

    m_commandBuffer.EndRendering();

    std::array<VkImageMemoryBarrier2, 2> barriers{};
    std::size_t barrierCount{0};
    for (const VkImage image : m_passImages)
    {
        if (image == VK_NULL_HANDLE)
        {
            continue;
        }
        barriers[barrierCount++] =
            VulkanImageBarrierBuilder{}
                .SetImage(image)
                .SetLayouts(VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL)
                .SetSrc(VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT, VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT)
                .SetDst(VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
                        VK_ACCESS_2_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT)
                .Build();
    }
    m_commandBuffer.PipelineBarrier(std::span{barriers}.first(barrierCount));
    m_passImages = {};
}

} // namespace QuakeLauncher::Renderer