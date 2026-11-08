#include "Renderer/CommandList.hpp"
#include "Renderer/Device.hpp"
#include "Renderer/Pipeline.hpp"

#include <string_view>

namespace
{

struct Vertex
{
    float x, y;
    float r, g, b;
};

constexpr std::array kVertices{Vertex{-0.5F, -0.5F, 1.0F, 0.0F, 0.0F},
                               Vertex{0.5F, -0.5F, 0.0F, 1.0F, 0.0F},
                               Vertex{0.5F, 0.5F, 0.0F, 0.0F, 1.0F},
                               Vertex{-0.5F, 0.5F, 1.0F, 1.0F, 0.0F}};

constexpr std::array<std::uint16_t, 6> kIndices{0, 1, 2, 2, 3, 0};

constexpr std::string_view kVertexShader{R"(
#version 450

layout(location = 0) in vec2 inPosition;
layout(location = 1) in vec3 inColor;
layout(location = 0) out vec3 vColor;

layout(push_constant) uniform 

void main()
{
    gl_Position = vec4(inPosition, 0.0, 1.0);
    vColor = inColor;
}
)"};

constexpr std::string_view kFragmentShader{R"(
#version 450

layout(location = 0) in vec3 vColor;
layout(location = 0) out vec4 outColor;

void main()
{
    outColor = vec4(vColor, 1.0);
}
)"};

} // namespace

extern "C" void Frame(Quake::Renderer::Device &device)
{
    constexpr auto kSamples{Quake::Renderer::SampleCount::X8};

    constexpr std::array attributes{Quake::Renderer::VertexAttribute{0, Quake::Renderer::VertexFormat::Float2, offsetof(Vertex, x)},
                                    Quake::Renderer::VertexAttribute{1, Quake::Renderer::VertexFormat::Float3, offsetof(Vertex, r)}};

    static Quake::Renderer::Pipeline &pipeline{
        device.CreatePipeline(Quake::Renderer::PipelineBuilder{}
                                  .SetVertexShader(kVertexShader)
                                  .SetFragmentShader(kFragmentShader)
                                  .SetCullMode(Quake::Renderer::CullMode::None)
                                  .SetSampleCount(kSamples)
                                  .SetVertexLayout({.stride = sizeof(Vertex), .attributes = attributes})
                                  .Build())};

    static Quake::Renderer::Buffer &vertexBuffer{
        device.CreateBuffer(Quake::Renderer::BufferBuilder{}
                                .SetUsage(Quake::Renderer::BufferUsage::Vertex)
                                .SetData(kVertices)
                                .Build())};
    static Quake::Renderer::Buffer &indexBuffer{
        device.CreateBuffer(Quake::Renderer::BufferBuilder{}
                                .SetUsage(Quake::Renderer::BufferUsage::Index)
                                .SetData(kIndices)
                                .Build())};

    Quake::Renderer::CommandList &commandList{device.BeginFrame()};

    Quake::Renderer::Texture &backbuffer{device.GetBackbuffer()};

    static Quake::Renderer::Texture &multisample{
        device.CreateTexture(Quake::Renderer::TextureBuilder{}
                                 .SetMatchBackbuffer()
                                 .SetFormat(backbuffer.GetFormat())
                                 .SetSampleCount(kSamples)
                                 .Build())};

    commandList.BeginRenderPass(Quake::Renderer::ColorAttachmentBuilder{}
                                    .SetTarget(multisample)
                                    .SetResolveTarget(backbuffer)
                                    .SetStoreOp(Quake::Renderer::StoreOp::DontCare)
                                    .SetClearColor(0.01F, 0.01F, 0.01F)
                                    .Build());
    commandList.SetPipeline(pipeline);
    commandList.SetVertexBuffer(vertexBuffer);
    commandList.SetIndexBuffer(indexBuffer, Quake::Renderer::IndexType::UInt16);
    commandList.DrawIndexed(kIndices.size());
    commandList.EndRenderPass();

    device.EndFrame();
}