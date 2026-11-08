#include <cstdlib>

#include "DynamicLibrary/Interface.hpp"

int main()
{
    // TODO: Make it work on Windows
    QuakeLauncher::DynamicLibrary::Interface library{"libQuake.so"};

    const auto frame{library.Find<void (*)()>("Frame")};
    frame();

    return EXIT_SUCCESS;
}
