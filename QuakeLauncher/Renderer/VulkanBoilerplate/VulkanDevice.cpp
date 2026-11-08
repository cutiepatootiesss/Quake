#include "VulkanDevice.hpp"

#include <algorithm>
#include <cstdint>
#include <optional>
#include <print>
#include <stdexcept>
#include <string_view>
#include <vector>
#include <volk.h>

#include "VulkanInstance.hpp"
#include "VulkanSurface.hpp"

namespace QuakeLauncher::Renderer
{
namespace
{

constexpr const char *kSwapchainExtension{VK_KHR_SWAPCHAIN_EXTENSION_NAME};

bool SupportsSwapchain(VkPhysicalDevice device)
{
    std::uint32_t count{0};
    ThrowIfFailed(vkEnumerateDeviceExtensionProperties(device, nullptr, &count, nullptr),
                  "vkEnumerateDeviceExtensionProperties");
    std::vector<VkExtensionProperties> extensions(count);
    ThrowIfFailed(vkEnumerateDeviceExtensionProperties(device, nullptr, &count, extensions.data()),
                  "vkEnumerateDeviceExtensionProperties");

    return std::ranges::any_of(extensions, [](const VkExtensionProperties &extension)
                               { return std::string_view{extension.extensionName} == kSwapchainExtension; });
}

std::optional<std::uint32_t> FindGraphicsQueueFamily(VkPhysicalDevice device, VkSurfaceKHR surface)
{
    std::uint32_t count{0};
    vkGetPhysicalDeviceQueueFamilyProperties(device, &count, nullptr);
    std::vector<VkQueueFamilyProperties> families(count);
    vkGetPhysicalDeviceQueueFamilyProperties(device, &count, families.data());

    for (std::uint32_t index{0}; index < count; ++index)
    {
        if ((families[index].queueFlags & VK_QUEUE_GRAPHICS_BIT) == 0)
        {
            continue;
        }

        VkBool32 presentSupported{VK_FALSE};
        ThrowIfFailed(vkGetPhysicalDeviceSurfaceSupportKHR(device, index, surface, &presentSupported),
                      "vkGetPhysicalDeviceSurfaceSupportKHR");
        if (presentSupported == VK_TRUE)
        {
            return index;
        }
    }
    return std::nullopt;
}

} // namespace

VulkanDevice::VulkanDevice(const VulkanInstance &instance, const VulkanSurface &surface)
{
    std::uint32_t deviceCount{0};
    ThrowIfFailed(vkEnumeratePhysicalDevices(instance.GetInstance(), &deviceCount, nullptr),
                  "vkEnumeratePhysicalDevices");
    std::vector<VkPhysicalDevice> candidates(deviceCount);
    ThrowIfFailed(vkEnumeratePhysicalDevices(instance.GetInstance(), &deviceCount, candidates.data()),
                  "vkEnumeratePhysicalDevices");

    for (const VkPhysicalDevice candidate : candidates)
    {
        VkPhysicalDeviceProperties properties{};
        vkGetPhysicalDeviceProperties(candidate, &properties);
        if (properties.apiVersion < kRequiredApiVersion)
        {
            continue;
        }

        if (!SupportsSwapchain(candidate))
        {
            continue;
        }

        const auto queueFamily{FindGraphicsQueueFamily(candidate, surface.GetSurface())};
        if (!queueFamily)
        {
            continue;
        }

        m_physicalDevice = candidate;
        m_graphicsQueueFamily = *queueFamily;

        std::println("Selected GPU: {}", static_cast<const char *>(properties.deviceName));
        break;
    }
    if (m_physicalDevice == VK_NULL_HANDLE)
    {
        throw std::runtime_error{"No supported physical devices"};
    }

    constexpr float queuePriority{1.0F};

    VkDeviceQueueCreateInfo queueCreateInfo{};
    queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    queueCreateInfo.queueFamilyIndex = m_graphicsQueueFamily;
    queueCreateInfo.queueCount = 1;
    queueCreateInfo.pQueuePriorities = &queuePriority;

    VkPhysicalDeviceVulkan13Features features13{};
    features13.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES;
    features13.dynamicRendering = VK_TRUE;
    features13.synchronization2 = VK_TRUE;

    VkPhysicalDeviceVulkan14Features features14{};
    features14.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_4_FEATURES;
    features14.maintenance5 = VK_TRUE;
    features13.pNext = &features14;

    VkDeviceCreateInfo deviceCreateInfo{};
    deviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    deviceCreateInfo.pNext = &features13;
    deviceCreateInfo.queueCreateInfoCount = 1;
    deviceCreateInfo.pQueueCreateInfos = &queueCreateInfo;
    deviceCreateInfo.enabledExtensionCount = 1;
    deviceCreateInfo.ppEnabledExtensionNames = &kSwapchainExtension;

    VkDevice device{VK_NULL_HANDLE};
    ThrowIfFailed(vkCreateDevice(m_physicalDevice, &deviceCreateInfo, nullptr, &device), "vkCreateDevice");

    m_logicalDevice.reset(device);
    volkLoadDevice(device);

    vkGetDeviceQueue(device, m_graphicsQueueFamily, 0, &m_graphicsQueue);
}

void VulkanDevice::WaitIdle() const
{
    ThrowIfFailed(vkDeviceWaitIdle(m_logicalDevice.get()), "vkDeviceWaitIdle");
}

} // namespace QuakeLauncher::Renderer