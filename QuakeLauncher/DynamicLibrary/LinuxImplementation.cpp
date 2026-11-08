#include "Interface.hpp"

#include <dlfcn.h>
#include <filesystem>
#include <stdexcept>
#include <string>
#include <string_view>

namespace QuakeLauncher::DynamicLibrary
{
namespace
{

[[nodiscard]] std::string LastError()
{
    const char *const message{dlerror()};
    return message != nullptr ? std::string{message} : std::string{"Unknown message"};
}

} // namespace

void Interface::HandleDeleter::operator()(void *handle) const noexcept
{
    static_cast<void>(dlclose(handle));
}

Interface::Interface(const std::filesystem::path &path) : m_handle{dlopen(path.c_str(), RTLD_NOW | RTLD_LOCAL)}
{
    if (m_handle == nullptr)
    {
        throw std::runtime_error{"Failed to load library: " + LastError()};
    }
}

SymbolAddress Interface::FindSymbol(std::string_view name) const
{
    // dlsym wants a null terminated string and a string view unfortunately does not guarantee that
    const std::string nameZ{name};
    // Clear the error.
    static_cast<void>(dlerror());

    void *const address{dlsym(m_handle.get(), nameZ.c_str())};

    if (const char *const error{dlerror()}; error != nullptr)
    {
        throw std::runtime_error{"Failed to find symbol: " + nameZ + ": " + error};
    }

    return reinterpret_cast<SymbolAddress>(address);
}

} // namespace QuakeLauncher::DynamicLibrary