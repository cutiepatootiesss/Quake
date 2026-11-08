#pragma once

#include <cstdint>
#include <span>
#include <stdexcept>
#include <string_view>

#include "Texture.hpp"

namespace Quake::Renderer
{

enum class PrimitiveTopology
{
    TriangleList,
    TriangleStrip,
    LineList,
};

enum class CullMode
{
    None,
    Front,
    Back,
};

enum class VertexFormat
{
    Float,
    Float2,
    Float3,
    Float4,
};

struct VertexAttribute
{
    std::uint32_t location;
    VertexFormat format;
    std::uint32_t offset;
};

struct VertexLayout
{
    std::uint32_t stride{0};
    std::span<const VertexAttribute> attributes;
};

class Pipeline
{
  public:
    virtual ~Pipeline() = default;

    Pipeline(const Pipeline &) = delete;
    Pipeline &operator=(const Pipeline &) = delete;
    Pipeline(Pipeline &&) = delete;
    Pipeline &operator=(Pipeline &&) = delete;

  protected:
    Pipeline() = default;
};

class PipelineDescriptor
{
  public:
    constexpr PipelineDescriptor(std::string_view vertexShader,
                                 std::string_view fragmentShader,
                                 PrimitiveTopology topology,
                                 CullMode cullMode,
                                 SampleCount sampleCount,
                                 VertexLayout vertexLayout) noexcept
        : m_vertexShader{vertexShader},
          m_fragmentShader{fragmentShader},
          m_topology{topology},
          m_cullMode{cullMode},
          m_sampleCount{sampleCount},
          m_vertexLayout{vertexLayout}
    {
    }

    [[nodiscard]] constexpr std::string_view GetVertexShader() const noexcept
    {
        return m_vertexShader;
    }

    [[nodiscard]] constexpr std::string_view GetFragmentShader() const noexcept
    {
        return m_fragmentShader;
    }

    [[nodiscard]] constexpr PrimitiveTopology GetTopology() const noexcept
    {
        return m_topology;
    }

    [[nodiscard]] constexpr CullMode GetCullMode() const noexcept
    {
        return m_cullMode;
    }

    [[nodiscard]] constexpr SampleCount GetSampleCount() const noexcept
    {
        return m_sampleCount;
    }

    [[nodiscard]] constexpr const VertexLayout &GetVertexLayout() const noexcept
    {
        return m_vertexLayout;
    }

  private:
    std::string_view m_vertexShader;
    std::string_view m_fragmentShader;
    PrimitiveTopology m_topology;
    CullMode m_cullMode;
    SampleCount m_sampleCount;
    VertexLayout m_vertexLayout;
};

class PipelineBuilder
{
  public:
    /// GLSL source
    /// Entrypoint is expected to be main
    constexpr PipelineBuilder &SetVertexShader(std::string_view glsl) noexcept
    {
        m_vertexShader = glsl;
        return *this;
    }

    /// GLSL source
    /// Entrypoint is expected to be main
    constexpr PipelineBuilder &SetFragmentShader(std::string_view glsl) noexcept
    {
        m_fragmentShader = glsl;
        return *this;
    }

    constexpr PipelineBuilder &SetTopology(PrimitiveTopology topology) noexcept
    {
        m_topology = topology;
        return *this;
    }

    constexpr PipelineBuilder &SetCullMode(CullMode cullMode) noexcept
    {
        m_cullMode = cullMode;
        return *this;
    }

    /// Must match the sample count of the render target the pipeline is used with.
    constexpr PipelineBuilder &SetSampleCount(SampleCount sampleCount) noexcept
    {
        m_sampleCount = sampleCount;
        return *this;
    }

    constexpr PipelineBuilder &SetVertexLayout(VertexLayout vertexLayout) noexcept
    {
        m_vertexLayout = vertexLayout;
        return *this;
    }

    [[nodiscard]] PipelineDescriptor Build() const
    {
        if (m_vertexShader.empty() || m_fragmentShader.empty())
        {
            throw std::logic_error{"A pipeline needs a vertex and a fragment shader"};
        }
        return PipelineDescriptor{m_vertexShader, m_fragmentShader, m_topology, m_cullMode, m_sampleCount, m_vertexLayout};
    }

  private:
    std::string_view m_vertexShader;
    std::string_view m_fragmentShader;
    PrimitiveTopology m_topology{PrimitiveTopology::TriangleList};
    CullMode m_cullMode{CullMode::Back};
    SampleCount m_sampleCount{SampleCount::X1};
    VertexLayout m_vertexLayout;
};

} // namespace Quake::Renderer