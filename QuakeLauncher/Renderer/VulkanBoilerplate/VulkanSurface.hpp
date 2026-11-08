#pragma once

#include <volk.h>

#include "../../SDL/SDL.hpp"
#include "VulkanInstance.hpp"

namespace QuakeLauncher::Renderer
{

class VulkanSurface
{
  public:
    explicit VulkanSurface(const VulkanInstance &instance, const SDL::Window &window);
    ~VulkanSurface();

    VulkanSurface(const VulkanSurface &) = delete;
    VulkanSurface &operator=(const VulkanSurface &) = delete;
    VulkanSurface(VulkanSurface &&) = delete;
    VulkanSurface &operator=(VulkanSurface &&) = delete;

    [[nodiscard]] VkSurfaceKHR GetSurface() const noexcept
    {
        return m_surface;
    }

  private:
    VkInstance m_instance{VK_NULL_HANDLE};
    VkSurfaceKHR m_surface{VK_NULL_HANDLE};
};

} // namespace QuakeLauncher::Renderer