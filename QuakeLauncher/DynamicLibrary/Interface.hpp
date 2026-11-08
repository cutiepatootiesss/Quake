#pragma once

#include <filesystem>
#include <memory>
#include <string_view>
#include <type_traits>

namespace QuakeLauncher::DynamicLibrary
{

using SymbolAddress = void (*)();

class Interface
{
  public:
    explicit Interface(const std::filesystem::path &path);

    Interface(const Interface &) = delete;
    Interface &operator=(const Interface &) = delete;
    Interface(Interface &&) noexcept = default;
    Interface &operator=(Interface &&) noexcept = default;

    ~Interface() = default;

    [[nodiscard]] SymbolAddress FindSymbol(std::string_view name) const;

    template <typename Function>
        requires(std::is_pointer_v<Function> && std::is_function_v<std::remove_pointer_t<Function>>)
    [[nodiscard]] Function Find(std::string_view name) const
    {
        // Doing this is pretty much defined.
        return reinterpret_cast<Function>(FindSymbol(name));
    }

  private:
    struct HandleDeleter
    {
        void operator()(void *handle) const noexcept;
    };

    std::unique_ptr<void, HandleDeleter> m_handle;
};

/// Appends extension that is used on this platform for dynamic libraries.
std::string FormatName(std::string_view original);

}; // namespace QuakeLauncher::DynamicLibrary