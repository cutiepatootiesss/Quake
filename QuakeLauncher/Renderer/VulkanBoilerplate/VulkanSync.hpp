#pragma once

#include <cstdint>
#include <memory>
#include <type_traits>
#include <volk.h>

#include "VulkanDevice.hpp"

namespace QuakeLauncher::Renderer
{

/// How many frames the CPU may record ahead of the GPU.
inline constexpr std::uint32_t kFramesInFlight{2};

class VulkanSemaphore
{
  public:
    explicit VulkanSemaphore(const VulkanDevice &device);
    ~VulkanSemaphore() = default;

    VulkanSemaphore(const VulkanSemaphore &) = delete;
    VulkanSemaphore &operator=(const VulkanSemaphore &) = delete;
    VulkanSemaphore(VulkanSemaphore &&) noexcept = default;
    VulkanSemaphore &operator=(VulkanSemaphore &&) noexcept = default;

    [[nodiscard]] VkSemaphore GetSemaphore() const noexcept
    {
        return m_semaphore.get();
    }

  private:
    struct SemaphoreDeleter
    {
        explicit SemaphoreDeleter(VkDevice owner) noexcept
            : device{owner}
        {
        }

        void operator()(VkSemaphore semaphore) const noexcept
        {
            vkDestroySemaphore(device, semaphore, nullptr);
        }

        VkDevice device;
    };

    std::unique_ptr<std::remove_pointer_t<VkSemaphore>, SemaphoreDeleter> m_semaphore;
};

class VulkanSemaphoreBuilder
{
  public:
    [[nodiscard]] VulkanSemaphore Build(const VulkanDevice &device) const
    {
        return VulkanSemaphore{device};
    }
};

class VulkanFence
{
  public:
    explicit VulkanFence(const VulkanDevice &device, bool signaled);
    ~VulkanFence() = default;

    VulkanFence(const VulkanFence &) = delete;
    VulkanFence &operator=(const VulkanFence &) = delete;
    VulkanFence(VulkanFence &&) noexcept = default;
    VulkanFence &operator=(VulkanFence &&) noexcept = default;

    void Wait() const;

    void Reset();

    [[nodiscard]] VkFence GetFence() const noexcept
    {
        return m_fence.get();
    }

  private:
    struct FenceDeleter
    {
        explicit FenceDeleter(VkDevice owner) noexcept
            : device{owner}
        {
        }

        void operator()(VkFence fence) const noexcept
        {
            vkDestroyFence(device, fence, nullptr);
        }

        VkDevice device;
    };

    VkDevice m_device{VK_NULL_HANDLE};
    std::unique_ptr<std::remove_pointer_t<VkFence>, FenceDeleter> m_fence;
};

class VulkanFenceBuilder
{
  public:
    VulkanFenceBuilder &SetSignaled(bool signaled = true) noexcept
    {
        m_signaled = signaled;
        return *this;
    }

    [[nodiscard]] VulkanFence Build(const VulkanDevice &device) const
    {
        return VulkanFence{device, m_signaled};
    }

  private:
    bool m_signaled{false};
};

} // namespace QuakeLauncher::Renderer