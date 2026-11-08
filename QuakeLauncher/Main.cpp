#include <cstdlib>
#include <print>

#include "DynamicLibrary/Interface.hpp"
#include "Renderer/VulkanDeviceImpl.hpp"
#include "SDL/SDL.hpp"

int main()
{
    QuakeLauncher::SDL::Context context;
    QuakeLauncher::SDL::Window window{context, "Quake", 1280, 720};

    QuakeLauncher::Renderer::VulkanDeviceImpl device{context, window};

    QuakeLauncher::DynamicLibrary::Interface library{QuakeLauncher::DynamicLibrary::FormatName("libQuake")};

    const auto Frame{library.Find<void (*)(Quake::Renderer::Device &)>("Frame")};
    // Get the first frame ready, since stuff like shader compilation happens exactly during this call.
    Frame(device);
    
    std::println("The window is ready to be shown");
    window.Show();

    while (!window.ShouldClose())
    {
        Frame(device);
    }

    return EXIT_SUCCESS;
}