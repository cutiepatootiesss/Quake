#pragma once

#include <cstddef>
#include <cstdint>
#include <memory>
#include <optional>
#include <span>
#include <type_traits>
#include <vector>
#include <volk.h>

#include "VulkanDevice.hpp"
#include "VulkanSync.hpp"

namespace QuakeLauncher::Renderer
{

class VulkanCommandBuffer
{
  public:
    explicit VulkanCommandBuffer(VkCommandBuffer commandBuffer) noexcept
        : m_commandBuffer{commandBuffer}
    {
    }

    /// Implicitly resets the buffer if the pool is resettable.
    void Begin(VkCommandBufferUsageFlags flags = 0);
    void End();

    void BeginRendering(const VkRect2D &renderArea, std::span<const VkRenderingAttachmentInfo> colorAttachments);
    void PushConstants(VkPipelineLayout layout, VkShaderStageFlags stages, std::span<const std::byte> data);
    void BindGraphicsPipeline(VkPipeline pipeline);
    void SetViewportAndScissor(VkExtent2D extent);
    void BindVertexBuffer(VkBuffer buffer);
    void BindIndexBuffer(VkBuffer buffer, VkIndexType indexType);
    void DrawIndexed(std::uint32_t indexCount);
    void Draw(std::uint32_t vertexCount);
    void EndRendering();

    void PipelineBarrier(std::span<const VkImageMemoryBarrier2> imageBarriers);

    [[nodiscard]] VkCommandBuffer GetCommandBuffer() const noexcept
    {
        return m_commandBuffer;
    }

  private:
    VkCommandBuffer m_commandBuffer;
};

class VulkanCommandPool
{
  public:
    explicit VulkanCommandPool(const VulkanDevice &device, std::uint32_t queueFamily, VkCommandPoolCreateFlags flags);
    ~VulkanCommandPool() = default;

    VulkanCommandPool(const VulkanCommandPool &) = delete;
    VulkanCommandPool &operator=(const VulkanCommandPool &) = delete;
    VulkanCommandPool(VulkanCommandPool &&) noexcept = default;
    VulkanCommandPool &operator=(VulkanCommandPool &&) noexcept = default;

    [[nodiscard]] std::vector<VulkanCommandBuffer> Allocate(std::uint32_t count);

    [[nodiscard]] VkCommandPool GetCommandPool() const noexcept
    {
        return m_commandPool.get();
    }

  private:
    struct CommandPoolDeleter
    {
        explicit CommandPoolDeleter(VkDevice owner) noexcept
            : device{owner}
        {
        }

        void operator()(VkCommandPool commandPool) const noexcept
        {
            vkDestroyCommandPool(device, commandPool, nullptr);
        }

        VkDevice device;
    };

    VkDevice m_device{VK_NULL_HANDLE};
    std::unique_ptr<std::remove_pointer_t<VkCommandPool>, CommandPoolDeleter> m_commandPool;
};

class VulkanCommandPoolBuilder
{
  public:
    VulkanCommandPoolBuilder &SetQueueFamily(std::uint32_t queueFamily) noexcept
    {
        m_queueFamily = queueFamily;
        return *this;
    }

    VulkanCommandPoolBuilder &SetResettable(bool resettable = true) noexcept
    {
        m_resettable = resettable;
        return *this;
    }

    VulkanCommandPoolBuilder &SetTransient(bool transient = true) noexcept
    {
        m_transient = transient;
        return *this;
    }

    [[nodiscard]] VulkanCommandPool Build(const VulkanDevice &device) const;

  private:
    std::optional<std::uint32_t> m_queueFamily;
    bool m_resettable{false};
    bool m_transient{false};
};

class VulkanColorAttachmentBuilder
{
  public:
    VulkanColorAttachmentBuilder() noexcept
    {
        m_attachment.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
        m_attachment.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        m_attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        m_attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    }

    VulkanColorAttachmentBuilder &SetImageView(VkImageView imageView) noexcept
    {
        m_attachment.imageView = imageView;
        return *this;
    }

    VulkanColorAttachmentBuilder &SetLoadOp(VkAttachmentLoadOp loadOp) noexcept
    {
        m_attachment.loadOp = loadOp;
        return *this;
    }

    VulkanColorAttachmentBuilder &SetStoreOp(VkAttachmentStoreOp storeOp) noexcept
    {
        m_attachment.storeOp = storeOp;
        return *this;
    }

    VulkanColorAttachmentBuilder &SetClearColor(float r, float g, float b, float a = 1.0F) noexcept
    {
        m_attachment.clearValue.color = VkClearColorValue{{r, g, b, a}};
        return *this;
    }

    VulkanColorAttachmentBuilder &SetResolve(VkImageView imageView) noexcept
    {
        m_attachment.resolveMode = VK_RESOLVE_MODE_AVERAGE_BIT;
        m_attachment.resolveImageView = imageView;
        m_attachment.resolveImageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        return *this;
    }

    [[nodiscard]] VkRenderingAttachmentInfo Build() const noexcept
    {
        return m_attachment;
    }

  private:
    VkRenderingAttachmentInfo m_attachment{};
};

class VulkanImageBarrierBuilder
{
  public:
    VulkanImageBarrierBuilder() noexcept
    {
        m_barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
        m_barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        m_barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        m_barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        m_barrier.subresourceRange.levelCount = 1;
        m_barrier.subresourceRange.layerCount = 1;
    }

    VulkanImageBarrierBuilder &SetImage(VkImage image) noexcept
    {
        m_barrier.image = image;
        return *this;
    }

    VulkanImageBarrierBuilder &SetLayouts(VkImageLayout oldLayout, VkImageLayout newLayout) noexcept
    {
        m_barrier.oldLayout = oldLayout;
        m_barrier.newLayout = newLayout;
        return *this;
    }

    /// The work that must finish before the barrier.
    VulkanImageBarrierBuilder &SetSrc(VkPipelineStageFlags2 stage, VkAccessFlags2 access) noexcept
    {
        m_barrier.srcStageMask = stage;
        m_barrier.srcAccessMask = access;
        return *this;
    }

    /// The work that must wait for the barrier.
    VulkanImageBarrierBuilder &SetDst(VkPipelineStageFlags2 stage, VkAccessFlags2 access) noexcept
    {
        m_barrier.dstStageMask = stage;
        m_barrier.dstAccessMask = access;
        return *this;
    }

    VulkanImageBarrierBuilder &SetAspectMask(VkImageAspectFlags aspectMask) noexcept
    {
        m_barrier.subresourceRange.aspectMask = aspectMask;
        return *this;
    }

    [[nodiscard]] VkImageMemoryBarrier2 Build() const noexcept
    {
        return m_barrier;
    }

  private:
    VkImageMemoryBarrier2 m_barrier{};
};

class VulkanSubmitBuilder
{
  public:
    VulkanSubmitBuilder &AddWait(const VulkanSemaphore &semaphore, VkPipelineStageFlags2 stage);
    VulkanSubmitBuilder &AddSignal(const VulkanSemaphore &semaphore, VkPipelineStageFlags2 stage);
    VulkanSubmitBuilder &AddCommandBuffer(const VulkanCommandBuffer &commandBuffer);

    void Submit(VkQueue queue) const;
    void Submit(VkQueue queue, const VulkanFence &fence) const;

  private:
    void SubmitImpl(VkQueue queue, VkFence fence) const;

    std::vector<VkSemaphoreSubmitInfo> m_waits;
    std::vector<VkCommandBufferSubmitInfo> m_commandBuffers;
    std::vector<VkSemaphoreSubmitInfo> m_signals;
};

} // namespace QuakeLauncher::Renderer