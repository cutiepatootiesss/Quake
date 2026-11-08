#include "SDL.hpp"

#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>
#include <cstdint>
#include <stdexcept>
#include <string>

namespace QuakeLauncher::SDL
{

Context::Context()
{
    if (!SDL_Init(SDL_INIT_VIDEO))
    {
        throw std::runtime_error{std::string{"SDL_Init: "} + SDL_GetError()};
    }
}

Context::~Context()
{
    SDL_Quit();
}

std::span<const char *const> Context::GetVulkanInstanceExtensions() const
{
    std::uint32_t count{0};
    const char *const *extensions{SDL_Vulkan_GetInstanceExtensions(&count)};
    if (extensions == nullptr)
    {
        throw std::runtime_error{std::string{"SDL_Vulkan_GetInstanceExtensions: "} + SDL_GetError()};
    }
    return {extensions, count};
}

void Window::WindowDeleter::operator()(SDL_Window *window) const noexcept
{
    SDL_DestroyWindow(window);
}

void Window::Show() const noexcept
{
    SDL_ShowWindow(m_window.get());
}

bool Window::ShouldClose() const noexcept
{
    SDL_Event event{};
    while (SDL_PollEvent(&event))
    {
        if (event.type == SDL_EVENT_QUIT)
        {
            return true;
        }
    }
    return false;
}

std::pair<int, int> Window::GetSize() const
{
    int width{0};
    int height{0};
    if (!SDL_GetWindowSizeInPixels(m_window.get(), &width, &height))
    {
        throw std::runtime_error{std::string{"SDL_GetWindowSizeInPixels: "} + SDL_GetError()};
    }

    return {width, height};
}

// Create the window hidden initially so there isn't just a frame of nothing while we're still initializing
// all our stuff.
static constexpr auto kWindowFlags{SDL_WINDOW_VULKAN | SDL_WINDOW_HIDDEN | SDL_WINDOW_RESIZABLE};

Window::Window(const Context &, const std::string &title, int width, int height)
{
    SDL_Window *window{SDL_CreateWindow(title.c_str(), width, height, kWindowFlags)};
    if (window == nullptr)
    {
        throw std::runtime_error{std::string{"SDL_CreateWindow: "} + SDL_GetError()};
    }
    m_window.reset(window);
}

} // namespace QuakeLauncher::SDL