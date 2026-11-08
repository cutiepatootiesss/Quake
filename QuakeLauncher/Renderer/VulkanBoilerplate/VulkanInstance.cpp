#include "VulkanInstance.hpp"

#include <cstdint>
#include <stdexcept>
#include <string>
#include <volk.h>

namespace QuakeLauncher::Renderer
{

VulkanInstance::VulkanInstance(std::span<const char *const> extensions)
{
    ThrowIfFailed(volkInitialize(), "volkInitialize");

    const std::uint32_t supportedApiVersion{volkGetInstanceVersion()};
    if (supportedApiVersion < kRequiredApiVersion)
    {
        throw std::runtime_error{"The installed Vulkan loader does not support Vulkan 1.4"};
    }

    VkApplicationInfo applicationInfo{};
    applicationInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    applicationInfo.apiVersion = kRequiredApiVersion;

    VkInstanceCreateInfo instanceCreateInfo{};
    instanceCreateInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    instanceCreateInfo.pApplicationInfo = &applicationInfo;
    instanceCreateInfo.enabledExtensionCount = static_cast<std::uint32_t>(extensions.size());
    instanceCreateInfo.ppEnabledExtensionNames = extensions.data();

    VkInstance instance{VK_NULL_HANDLE};
    ThrowIfFailed(vkCreateInstance(&instanceCreateInfo, nullptr, &instance), "vkCreateInstance");

    m_instance.reset(instance);
    volkLoadInstance(instance);
}

} // namespace QuakeLauncher::Renderer