#pragma once

#include "CommandList.hpp"
#include "Pipeline.hpp"
#include "Texture.hpp"

namespace Quake::Renderer
{

class Device
{
  public:
    virtual ~Device() = default;

    [[nodiscard]] virtual Pipeline &CreatePipeline(const PipelineDescriptor &descriptor) = 0;
    [[nodiscard]] virtual Texture &CreateTexture(const TextureDescriptor &descriptor) = 0;
    [[nodiscard]] virtual Buffer &CreateBuffer(const BufferDescriptor &descriptor) = 0;

    /// The current swapchain image
    /// Only valid between BeginFrame() and EndFrame().
    [[nodiscard]] virtual Texture &GetBackbuffer() = 0;

    virtual CommandList &BeginFrame() = 0;
    virtual void EndFrame() = 0;
};

} // namespace Quake::Renderer