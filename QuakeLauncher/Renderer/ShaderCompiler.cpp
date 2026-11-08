#include "ShaderCompiler.hpp"

#include <SPIRV/GlslangToSpv.h>
#include <chrono>
#include <cstdint>
#include <glslang/Public/ResourceLimits.h>
#include <glslang/Public/ShaderLang.h>
#include <print>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace QuakeLauncher::Renderer
{
namespace
{

constexpr int kClientInputSemanticsVersion{100};
constexpr int kDefaultGlslVersion{450};

[[nodiscard]] EShLanguage ToGlslang(ShaderStage stage)
{
    switch (stage)
    {
    case ShaderStage::Vertex:
        return EShLangVertex;
    case ShaderStage::Fragment:
        return EShLangFragment;
    }
    throw std::invalid_argument{"Unknown ShaderStage"};
}

[[nodiscard]] std::string_view ToString(ShaderStage stage)
{
    return stage == ShaderStage::Vertex ? "Vertex" : "Fragment";
}

} // namespace

ShaderCompiler::ShaderCompiler()
{
    if (!glslang::InitializeProcess())
    {
        throw std::runtime_error{"glslang::InitializeProcess failed"};
    }
}

ShaderCompiler::~ShaderCompiler()
{
    glslang::FinalizeProcess();
}

std::vector<std::uint32_t> ShaderCompiler::Compile(ShaderStage stage, std::string_view source) const
{
    std::println("Compiling a {} shader...", ToString(stage));
    // TODO: I think we should have some sort of a timing utility.
    const auto start{std::chrono::steady_clock::now()};

    const EShLanguage language{ToGlslang(stage)};
    const auto messages{static_cast<EShMessages>(EShMsgSpvRules | EShMsgVulkanRules)};

    const char *text{source.data()};
    const int length{static_cast<int>(source.size())};

    glslang::TShader shader{language};
    shader.setStringsWithLengths(&text, &length, 1);
    shader.setEnvInput(glslang::EShSourceGlsl, language, glslang::EShClientVulkan, kClientInputSemanticsVersion);
    shader.setEnvClient(glslang::EShClientVulkan, glslang::EShTargetVulkan_1_4);
    shader.setEnvTarget(glslang::EShTargetSpv, glslang::EShTargetSpv_1_6);

    if (!shader.parse(GetDefaultResources(), kDefaultGlslVersion, false, messages))
    {
        throw std::runtime_error{std::string{ToString(stage)} + " shader failed to compile:\n" + shader.getInfoLog()};
    }

    glslang::TProgram program;
    program.addShader(&shader);
    if (!program.link(messages))
    {
        throw std::runtime_error{std::string{ToString(stage)} + " shader failed to link:\n" + program.getInfoLog()};
    }

    std::vector<std::uint32_t> spirv;
    glslang::GlslangToSpv(*program.getIntermediate(language), spirv);

    const std::chrono::duration<double, std::milli> elapsed{std::chrono::steady_clock::now() - start};
    std::println("Done! Took {:.2}", elapsed);
    return spirv;
}

} // namespace QuakeLauncher::Renderer