#pragma once

#include <array>
#include <cstdint>
#include <volk.h>

#include "../../Quake/Renderer/CommandList.hpp"
#include "VulkanBoilerplate/VulkanCmd.hpp"
#include "VulkanTextureImpl.hpp"

namespace QuakeLauncher::Renderer
{

class VulkanCommandListImpl final : public Quake::Renderer::CommandList
{
  public:
    VulkanCommandListImpl() = default;
    ~VulkanCommandListImpl() override = default;

    VulkanCommandListImpl(const VulkanCommandListImpl &) = delete;
    VulkanCommandListImpl &operator=(const VulkanCommandListImpl &) = delete;
    VulkanCommandListImpl(VulkanCommandListImpl &&) = delete;
    VulkanCommandListImpl &operator=(VulkanCommandListImpl &&) = delete;

    void BeginRenderPass(const Quake::Renderer::ColorAttachment &colorAttachment) override;
    void PushConstantsBytes(std::span<const std::byte> data) override;
    void SetPipeline(const Quake::Renderer::Pipeline &pipeline) override;
    void SetVertexBuffer(const Quake::Renderer::Buffer &buffer) override;
    void SetIndexBuffer(const Quake::Renderer::Buffer &buffer, Quake::Renderer::IndexType indexType) override;
    void DrawIndexed(std::uint32_t indexCount) override;
    void Draw(std::uint32_t vertexCount) override;
    void EndRenderPass() override;

    void Begin(VulkanCommandBuffer commandBuffer, const VulkanTextureImpl &backbuffer);
    void End();

    void Skip() noexcept
    {
        m_recording = false;
    }

    [[nodiscard]] const VulkanCommandBuffer &GetCommandBuffer() const noexcept
    {
        return m_commandBuffer;
    }

  private:
    VulkanCommandBuffer m_commandBuffer{VK_NULL_HANDLE};
    VkPipelineLayout m_pipelineLayout{VK_NULL_HANDLE}; // Of the currently bound pipeline.
    const VulkanTextureImpl *m_backbuffer{nullptr};
    std::array<VkImage, 2> m_passImages{}; // Target and resolve target of the current pass.
    bool m_recording{false};
};

} // namespace QuakeLauncher::Renderer