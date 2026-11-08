#pragma once

#include "CommandList.hpp"

namespace Quake::Renderer
{

class Device
{
  public:
    virtual ~Device() = default;

    virtual CommandList &BeginFrame() = 0;
    virtual void EndFrame() = 0;
};

} // namespace Quake::Renderer