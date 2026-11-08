#include "VulkanCmd.hpp"

#include <cstdint>
#include <span>
#include <vector>
#include <volk.h>

namespace QuakeLauncher::Renderer
{
namespace
{

[[nodiscard]] VkSemaphoreSubmitInfo MakeSemaphoreSubmitInfo(const VulkanSemaphore &semaphore,
                                                            VkPipelineStageFlags2 stage) noexcept
{
    VkSemaphoreSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO;
    submitInfo.semaphore = semaphore.GetSemaphore();
    submitInfo.stageMask = stage;
    return submitInfo;
}

} // namespace

void VulkanCommandBuffer::Begin(VkCommandBufferUsageFlags flags)
{
    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = flags;
    ThrowIfFailed(vkBeginCommandBuffer(m_commandBuffer, &beginInfo), "vkBeginCommandBuffer");
}

void VulkanCommandBuffer::End()
{
    ThrowIfFailed(vkEndCommandBuffer(m_commandBuffer), "vkEndCommandBuffer");
}

void VulkanCommandBuffer::BeginRendering(const VkRect2D &renderArea,
                                         std::span<const VkRenderingAttachmentInfo> colorAttachments)
{
    VkRenderingInfo renderingInfo{};
    renderingInfo.sType = VK_STRUCTURE_TYPE_RENDERING_INFO;
    renderingInfo.renderArea = renderArea;
    renderingInfo.layerCount = 1;
    renderingInfo.colorAttachmentCount = static_cast<std::uint32_t>(colorAttachments.size());
    renderingInfo.pColorAttachments = colorAttachments.data();
    vkCmdBeginRendering(m_commandBuffer, &renderingInfo);
}

void VulkanCommandBuffer::PushConstants(VkPipelineLayout layout, VkShaderStageFlags stages,
                                        std::span<const std::byte> data)
{
    VkPushConstantsInfo info{};
    info.sType = VK_STRUCTURE_TYPE_PUSH_CONSTANTS_INFO;
    info.layout = layout;
    info.stageFlags = stages;
    info.offset = 0;
    info.size = static_cast<std::uint32_t>(data.size());
    info.pValues = data.data();
    vkCmdPushConstants2(m_commandBuffer, &info);
}

void VulkanCommandBuffer::BindGraphicsPipeline(VkPipeline pipeline)
{
    vkCmdBindPipeline(m_commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
}

void VulkanCommandBuffer::SetViewportAndScissor(VkExtent2D extent)
{
    const VkViewport viewport{0.0F, 0.0F, static_cast<float>(extent.width), static_cast<float>(extent.height),
                              0.0F, 1.0F};
    const VkRect2D scissor{{0, 0}, extent};
    vkCmdSetViewport(m_commandBuffer, 0, 1, &viewport);
    vkCmdSetScissor(m_commandBuffer, 0, 1, &scissor);
}

void VulkanCommandBuffer::BindVertexBuffer(VkBuffer buffer)
{
    constexpr VkDeviceSize offset{0};
    vkCmdBindVertexBuffers(m_commandBuffer, 0, 1, &buffer, &offset);
}

void VulkanCommandBuffer::BindIndexBuffer(VkBuffer buffer, VkIndexType indexType)
{
    vkCmdBindIndexBuffer(m_commandBuffer, buffer, 0, indexType);
}

void VulkanCommandBuffer::DrawIndexed(std::uint32_t indexCount)
{
    vkCmdDrawIndexed(m_commandBuffer, indexCount, 1, 0, 0, 0);
}

void VulkanCommandBuffer::Draw(std::uint32_t vertexCount)
{
    vkCmdDraw(m_commandBuffer, vertexCount, 1, 0, 0);
}

void VulkanCommandBuffer::EndRendering()
{
    vkCmdEndRendering(m_commandBuffer);
}

void VulkanCommandBuffer::PipelineBarrier(std::span<const VkImageMemoryBarrier2> imageBarriers)
{
    VkDependencyInfo dependencyInfo{};
    dependencyInfo.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
    dependencyInfo.imageMemoryBarrierCount = static_cast<std::uint32_t>(imageBarriers.size());
    dependencyInfo.pImageMemoryBarriers = imageBarriers.data();
    vkCmdPipelineBarrier2(m_commandBuffer, &dependencyInfo);
}

VulkanCommandPool::VulkanCommandPool(const VulkanDevice &device, std::uint32_t queueFamily,
                                     VkCommandPoolCreateFlags flags)
    : m_device{device.GetLogicalDevice()},
      m_commandPool{nullptr, CommandPoolDeleter{m_device}}
{
    VkCommandPoolCreateInfo commandPoolCreateInfo{};
    commandPoolCreateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    commandPoolCreateInfo.flags = flags;
    commandPoolCreateInfo.queueFamilyIndex = queueFamily;

    VkCommandPool commandPool{VK_NULL_HANDLE};
    ThrowIfFailed(vkCreateCommandPool(m_device, &commandPoolCreateInfo, nullptr, &commandPool), "vkCreateCommandPool");
    m_commandPool.reset(commandPool);
}

std::vector<VulkanCommandBuffer> VulkanCommandPool::Allocate(std::uint32_t count)
{
    VkCommandBufferAllocateInfo allocateInfo{};
    allocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocateInfo.commandPool = m_commandPool.get();
    allocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocateInfo.commandBufferCount = count;

    std::vector<VkCommandBuffer> handles(count);
    ThrowIfFailed(vkAllocateCommandBuffers(m_device, &allocateInfo, handles.data()), "vkAllocateCommandBuffers");

    std::vector<VulkanCommandBuffer> commandBuffers;
    commandBuffers.reserve(handles.size());
    for (const VkCommandBuffer handle : handles)
    {
        commandBuffers.emplace_back(handle);
    }
    return commandBuffers;
}

VulkanCommandPool VulkanCommandPoolBuilder::Build(const VulkanDevice &device) const
{
    VkCommandPoolCreateFlags flags{0};
    if (m_resettable)
    {
        flags |= VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    }
    if (m_transient)
    {
        flags |= VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;
    }

    return VulkanCommandPool{device, m_queueFamily.value_or(device.GetGraphicsQueueFamily()), flags};
}

VulkanSubmitBuilder &VulkanSubmitBuilder::AddWait(const VulkanSemaphore &semaphore, VkPipelineStageFlags2 stage)
{
    m_waits.push_back(MakeSemaphoreSubmitInfo(semaphore, stage));
    return *this;
}

VulkanSubmitBuilder &VulkanSubmitBuilder::AddSignal(const VulkanSemaphore &semaphore, VkPipelineStageFlags2 stage)
{
    m_signals.push_back(MakeSemaphoreSubmitInfo(semaphore, stage));
    return *this;
}

VulkanSubmitBuilder &VulkanSubmitBuilder::AddCommandBuffer(const VulkanCommandBuffer &commandBuffer)
{
    VkCommandBufferSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO;
    submitInfo.commandBuffer = commandBuffer.GetCommandBuffer();
    m_commandBuffers.push_back(submitInfo);
    return *this;
}

void VulkanSubmitBuilder::Submit(VkQueue queue) const
{
    SubmitImpl(queue, VK_NULL_HANDLE);
}

void VulkanSubmitBuilder::Submit(VkQueue queue, const VulkanFence &fence) const
{
    SubmitImpl(queue, fence.GetFence());
}

void VulkanSubmitBuilder::SubmitImpl(VkQueue queue, VkFence fence) const
{
    VkSubmitInfo2 submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO_2;
    submitInfo.waitSemaphoreInfoCount = static_cast<std::uint32_t>(m_waits.size());
    submitInfo.pWaitSemaphoreInfos = m_waits.data();
    submitInfo.commandBufferInfoCount = static_cast<std::uint32_t>(m_commandBuffers.size());
    submitInfo.pCommandBufferInfos = m_commandBuffers.data();
    submitInfo.signalSemaphoreInfoCount = static_cast<std::uint32_t>(m_signals.size());
    submitInfo.pSignalSemaphoreInfos = m_signals.data();

    ThrowIfFailed(vkQueueSubmit2(queue, 1, &submitInfo, fence), "vkQueueSubmit2");
}

} // namespace QuakeLauncher::Renderer