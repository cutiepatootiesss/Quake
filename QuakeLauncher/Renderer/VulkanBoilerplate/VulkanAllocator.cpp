#include "VulkanAllocator.hpp"

#include <volk.h>

namespace QuakeLauncher::Renderer
{

VulkanAllocator::VulkanAllocator(const VulkanInstance &instance, const VulkanDevice &device)
{
    VmaVulkanFunctions functions{};
    functions.vkGetInstanceProcAddr = vkGetInstanceProcAddr;
    functions.vkGetDeviceProcAddr = vkGetDeviceProcAddr;

    VmaAllocatorCreateInfo createInfo{};
    createInfo.physicalDevice = device.GetPhysicalDevice();
    createInfo.device = device.GetLogicalDevice();
    createInfo.instance = instance.GetInstance();
    // If this is higher than 1.3 then an assertion triggers...
    createInfo.vulkanApiVersion = VK_API_VERSION_1_3;
    createInfo.pVulkanFunctions = &functions;

    VmaAllocator allocator{nullptr};
    ThrowIfFailed(vmaCreateAllocator(&createInfo, &allocator), "vmaCreateAllocator");
    m_allocator.reset(allocator);
}

} // namespace QuakeLauncher::Renderer