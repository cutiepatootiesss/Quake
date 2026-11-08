#include "VulkanSurface.hpp"

#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>
#include <stdexcept>
#include <string>

#include "../../SDL/SDL.hpp"
#include "VulkanInstance.hpp"

namespace QuakeLauncher::Renderer
{

VulkanSurface::VulkanSurface(const VulkanInstance &instance, const SDL::Window &window)
    : m_instance{instance.GetInstance()}
{
    if (!SDL_Vulkan_CreateSurface(window.GetWindow(), m_instance, nullptr, &m_surface))
    {
        throw std::runtime_error{std::string{"SDL_Vulkan_CreateSurface: "} + SDL_GetError()};
    }
}

VulkanSurface::~VulkanSurface()
{
    SDL_Vulkan_DestroySurface(m_instance, m_surface, nullptr);
}

} // namespace QuakeLauncher::Renderer