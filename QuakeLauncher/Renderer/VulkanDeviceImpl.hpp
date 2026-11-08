#pragma once

#include <cstdint>
#include <memory>
#include <vector>

#include "../../Quake/Renderer/Device.hpp"
#include "../../Quake/Renderer/Pipeline.hpp"
#include "../SDL/SDL.hpp"
#include "ShaderCompiler.hpp"
#include "VulkanBoilerplate/VulkanCmd.hpp"
#include "VulkanBoilerplate/VulkanDevice.hpp"
#include "VulkanBoilerplate/VulkanInstance.hpp"
#include "VulkanBoilerplate/VulkanSurface.hpp"
#include "VulkanBoilerplate/VulkanSwapchain.hpp"
#include "VulkanBoilerplate/VulkanSync.hpp"
#include "VulkanBufferImpl.hpp"
#include "VulkanCommandListImpl.hpp"
#include "VulkanPipelineImpl.hpp"

namespace QuakeLauncher::Renderer
{

class VulkanDeviceImpl final : public Quake::Renderer::Device
{
  public:
    explicit VulkanDeviceImpl(const SDL::Context &context, const SDL::Window &window);
    ~VulkanDeviceImpl() override;

    Quake::Renderer::Pipeline &CreatePipeline(const Quake::Renderer::PipelineDescriptor &descriptor) override;
    Quake::Renderer::Texture &CreateTexture(const Quake::Renderer::TextureDescriptor &descriptor) override;
    Quake::Renderer::Buffer &CreateBuffer(const Quake::Renderer::BufferDescriptor &descriptor) override;

    Quake::Renderer::Texture &GetBackbuffer() override;

    VulkanDeviceImpl(const VulkanDeviceImpl &) = delete;
    VulkanDeviceImpl &operator=(const VulkanDeviceImpl &) = delete;
    VulkanDeviceImpl(VulkanDeviceImpl &&) = delete;
    VulkanDeviceImpl &operator=(VulkanDeviceImpl &&) = delete;

    Quake::Renderer::CommandList &BeginFrame() override;
    void EndFrame() override;

  private:
    [[nodiscard]] bool AcquireImage();

    [[nodiscard]] VulkanImage MakeImage(Quake::Renderer::TextureFormat format, Quake::Renderer::SampleCount samples,
                                        VkExtent2D extent) const;

    const SDL::Window &m_window;

    ShaderCompiler m_shaderCompiler;

    VulkanInstance m_instance;
    VulkanSurface m_surface;
    VulkanDevice m_device;
    VulkanAllocator m_allocator;
    VulkanSwapchain m_swapchain;

    VulkanCommandPool m_commandPool;
    std::vector<VulkanCommandBuffer> m_commandBuffers; // Per frame in flight
    VulkanCommandListImpl m_commandList;
    std::vector<VulkanSemaphore> m_imageAvailable; // Per frame in flight
    std::vector<VulkanFence> m_inFlight;           // Per frame in flight
    std::vector<VulkanSemaphore> m_renderFinished; // Per swapchain image
    std::uint32_t m_frameIndex{0};
    std::uint32_t m_imageIndex{0};

    std::vector<std::unique_ptr<VulkanPipelineImpl>> m_pipelines;
    std::vector<std::unique_ptr<VulkanTextureImpl>> m_textures;
    std::vector<std::unique_ptr<VulkanBufferImpl>> m_buffers;
    std::vector<std::unique_ptr<VulkanTextureImpl>> m_backbuffers;

    bool m_frameActive{false};
    bool m_needsRecreate{true};
};

} // namespace QuakeLauncher::Renderer