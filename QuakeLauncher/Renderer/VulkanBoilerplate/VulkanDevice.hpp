#pragma once

#include <cstdint>
#include <memory>
#include <type_traits>
#include <volk.h>

#include "VulkanInstance.hpp"
#include "VulkanSurface.hpp"

namespace QuakeLauncher::Renderer
{

class VulkanDevice
{
  public:
    explicit VulkanDevice(const VulkanInstance &instance, const VulkanSurface &surface);

    VulkanDevice(const VulkanDevice &) = delete;
    VulkanDevice &operator=(const VulkanDevice &) = delete;
    VulkanDevice &operator=(VulkanDevice &&other) = delete;

    [[nodiscard]] VkPhysicalDevice GetPhysicalDevice() const noexcept
    {
        return m_physicalDevice;
    }

    [[nodiscard]] VkDevice GetLogicalDevice() const noexcept
    {
        return m_logicalDevice.get();
    }

    [[nodiscard]] std::uint32_t GetGraphicsQueueFamily() const noexcept
    {
        return m_graphicsQueueFamily;
    }

    [[nodiscard]] VkQueue GetGraphicsQueue() const noexcept
    {
        return m_graphicsQueue;
    }

    void WaitIdle() const;

  private:
    struct DeviceDeleter
    {
        void operator()(VkDevice device) const noexcept
        {
            vkDestroyDevice(device, nullptr);
        }
    };

    VkPhysicalDevice m_physicalDevice{VK_NULL_HANDLE};
    std::uint32_t m_graphicsQueueFamily{0};
    std::unique_ptr<std::remove_pointer_t<VkDevice>, DeviceDeleter> m_logicalDevice;
    VkQueue m_graphicsQueue{VK_NULL_HANDLE};
};

} // namespace QuakeLauncher::Renderer