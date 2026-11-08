#include "Interface.hpp"

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <Windows.h>

#include <filesystem>
#include <string>
#include <string_view>
#include <system_error>

namespace QuakeLauncher::DynamicLibrary
{
namespace
{

[[nodiscard]] std::error_code LastError() noexcept
{
    return {static_cast<int>(GetLastError()), std::system_category()};
}

} // namespace

void Interface::HandleDeleter::operator()(void *handle) const noexcept
{
    // A destructor can't throw, so the result is intentionally discarded.
    static_cast<void>(FreeLibrary(static_cast<HMODULE>(handle)));
}

Interface::Interface(const std::filesystem::path &path) : m_handle{LoadLibraryW(path.c_str())}
{
    if (m_handle == nullptr)
    {
        throw std::system_error{LastError(), "Failed to load library"};
    }
}

SymbolAddress Interface::FindSymbol(std::string_view name) const
{
    // GetProcAddress wants a null terminated string and a string view unfortunately does not guarantee that
    const std::string nameZ{name};

    const FARPROC address{GetProcAddress(static_cast<HMODULE>(m_handle.get()), nameZ.c_str())};
    if (address == nullptr)
    {
        throw std::system_error{LastError(), "Failed to find symbol: " + nameZ};
    }

    // Both these guys are function pointer so this also is pretty muh defined.
    return reinterpret_cast<SymbolAddress>(address);
}

/// Appends extension that is used on this platform for dynamic libraries.
std::string FormatName(std::string_view original)
{
    return std::string{original} + ".dll";
}

} // namespace QuakeLauncher::DynamicLibrary