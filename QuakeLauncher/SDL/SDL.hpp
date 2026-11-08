#pragma once

#include <memory>
#include <span>
#include <string>
#include <utility>

struct SDL_Window;

namespace QuakeLauncher::SDL
{

class Context
{
  public:
    explicit Context();
    ~Context();

    Context(const Context &) = delete;
    Context &operator=(const Context &) = delete;
    Context(Context &&) = delete;
    Context &operator=(Context &&) = delete;

    [[nodiscard]] std::span<const char *const> GetVulkanInstanceExtensions() const;
};

class Window
{
  public:
    explicit Window(const Context &context, const std::string &title, int width, int height);

    Window(const Window &) = delete;
    Window &operator=(const Window &) = delete;
    Window &operator=(Window &&other) = delete;

    void Show() const noexcept;

    [[nodiscard]] bool ShouldClose() const noexcept;

    [[nodiscard]] std::pair<int, int> GetSize() const;

    [[nodiscard]] SDL_Window *GetWindow() const noexcept
    {
        return m_window.get();
    }

  private:
    struct WindowDeleter
    {
        void operator()(SDL_Window *window) const noexcept;
    };

    std::unique_ptr<SDL_Window, WindowDeleter> m_window;
};

} // namespace QuakeLauncher::SDL