#pragma once

#include <cstdint>
#include <string_view>
#include <vector>

namespace QuakeLauncher::Renderer
{

enum class ShaderStage
{
    Vertex,
    Fragment,
};

/// Compiles GLSL to SPIR-V
class ShaderCompiler
{
  public:
    ShaderCompiler();
    ~ShaderCompiler();

    ShaderCompiler(const ShaderCompiler &) = delete;
    ShaderCompiler &operator=(const ShaderCompiler &) = delete;
    ShaderCompiler(ShaderCompiler &&) = delete;
    ShaderCompiler &operator=(ShaderCompiler &&) = delete;

    [[nodiscard]] std::vector<std::uint32_t> Compile(ShaderStage stage, std::string_view source) const;
};

} // namespace QuakeLauncher::Renderer