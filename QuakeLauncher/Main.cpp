#include <cstdlib>
#include <print>

#include "DynamicLibrary/Interface.hpp"

int main()
{
    // TODO this does not work yet because the library is not being ubilt
    QuakeLauncher::DynamicLibrary::Interface library{"Quake.dll"};

    std::println("Hello, World!");
    return EXIT_SUCCESS;
}
