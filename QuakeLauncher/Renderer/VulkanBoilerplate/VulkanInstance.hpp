#pragma once

#include <cstdint>
#include <memory>
#include <span>
#include <type_traits>
#include <volk.h>

namespace QuakeLauncher::Renderer
{

static constexpr std::uint32_t kRequiredApiVersion{VK_API_VERSION_1_4};

class VulkanInstance
{
  public:
    explicit VulkanInstance(std::span<const char *const> extensions);

    VulkanInstance(const VulkanInstance &) = delete;
    VulkanInstance &operator=(const VulkanInstance &) = delete;
    VulkanInstance &operator=(VulkanInstance &&other) = delete;

    [[nodiscard]] VkInstance GetInstance() const noexcept
    {
        return m_instance.get();
    }

  private:
    struct InstanceDeleter
    {
        void operator()(VkInstance instance) const noexcept
        {
            vkDestroyInstance(instance, nullptr);
        }
    };

    std::unique_ptr<std::remove_pointer_t<VkInstance>, InstanceDeleter> m_instance;
};

inline void ThrowIfFailed(VkResult result, std::string_view call)
{
    if (result != VK_SUCCESS)
    {
        throw std::runtime_error{std::string{call} + ": " + std::to_string(result)};
    }
}

} // namespace QuakeLauncher::Renderer