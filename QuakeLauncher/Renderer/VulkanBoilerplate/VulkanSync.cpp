#include "VulkanSync.hpp"

#include <cstdint>
#include <limits>
#include <volk.h>

namespace QuakeLauncher::Renderer
{

VulkanSemaphore::VulkanSemaphore(const VulkanDevice &device)
    : m_semaphore{nullptr, SemaphoreDeleter{device.GetLogicalDevice()}}
{
    VkSemaphoreCreateInfo semaphoreCreateInfo{};
    semaphoreCreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    VkSemaphore semaphore{VK_NULL_HANDLE};
    ThrowIfFailed(vkCreateSemaphore(device.GetLogicalDevice(), &semaphoreCreateInfo, nullptr, &semaphore),
                  "vkCreateSemaphore");
    m_semaphore.reset(semaphore);
}

VulkanFence::VulkanFence(const VulkanDevice &device, bool signaled)
    : m_device{device.GetLogicalDevice()},
      m_fence{nullptr, FenceDeleter{m_device}}
{
    VkFenceCreateInfo fenceCreateInfo{};
    fenceCreateInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    if (signaled)
    {
        fenceCreateInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;
    }

    VkFence fence{VK_NULL_HANDLE};
    ThrowIfFailed(vkCreateFence(m_device, &fenceCreateInfo, nullptr, &fence), "vkCreateFence");
    m_fence.reset(fence);
}

void VulkanFence::Wait() const
{
    const VkFence fence{m_fence.get()};
    ThrowIfFailed(vkWaitForFences(m_device, 1, &fence, VK_TRUE, std::numeric_limits<std::uint64_t>::max()),
                  "vkWaitForFences");
}

void VulkanFence::Reset()
{
    const VkFence fence{m_fence.get()};
    ThrowIfFailed(vkResetFences(m_device, 1, &fence), "vkResetFences");
}

} // namespace QuakeLauncher::Renderer