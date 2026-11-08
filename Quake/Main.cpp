#include <print>

#include "Renderer/CommandList.hpp"
#include "Renderer/Device.hpp"

extern "C" void Frame(Quake::Renderer::Device &device)
{
    std::println("Hello, World!");
}