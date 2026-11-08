#pragma once

#include <span>

namespace Quake::Renderer
{

/// What happens to color attachment contents at the start of the frame.
enum class LoadOp
{
    Load,
    Clear,
    DontCare,
};

/// What happens to attachment contents at the end of a render pass.
enum class StoreOp
{
    Store,
    DontCare,
};

// TODO: Put it in a shared math header once w ehave more types
struct Color
{
    float r{0.0F};
    float g{0.0F};
    float b{0.0F};
    float a{0.0F};
};

/// See also: ColorAttachmentBuilder
class ColorAttachment
{
  public:
    constexpr ColorAttachment(LoadOp loadOp, StoreOp storeOp, Color clearColor) noexcept
        : m_loadOp{loadOp}, m_storeOp{storeOp}, m_clearColor{clearColor}
    {
    }

    [[nodiscard]] constexpr LoadOp GetLoadOp() const noexcept
    {
        return m_loadOp;
    }

    [[nodiscard]] constexpr StoreOp GetStoreOp() const noexcept
    {
        return m_storeOp;
    }

    [[nodiscard]] constexpr const Color &GetClearColor() const noexcept
    {
        return m_clearColor;
    }

  private:
    LoadOp m_loadOp;
    StoreOp m_storeOp;
    Color m_clearColor;
};

class ColorAttachmentBuilder
{
  public:
    constexpr ColorAttachmentBuilder &SetLoadOp(LoadOp loadOp) noexcept
    {
        m_loadOp = loadOp;
        return *this;
    }

    constexpr ColorAttachmentBuilder &SetStoreOp(StoreOp storeOp) noexcept
    {
        m_storeOp = storeOp;
        return *this;
    }

    constexpr ColorAttachmentBuilder &SetClearColor(Color clearColor) noexcept
    {
        m_clearColor = clearColor;
        return *this;
    }

    constexpr ColorAttachmentBuilder &SetClearColor(float r, float g, float b, float a = 1.0F) noexcept
    {
        m_clearColor = Color{r, g, b, a};
        return *this;
    }

    [[nodiscard]] constexpr ColorAttachment Build() const noexcept
    {
        return ColorAttachment{m_loadOp, m_storeOp, m_clearColor};
    }

  private:
    LoadOp m_loadOp{LoadOp::Clear};
    StoreOp m_storeOp{StoreOp::Store};
    Color m_clearColor{};
};

class CommandList
{
  public:
    virtual ~CommandList() = default;

    virtual void BeginRenderPass(std::span<const ColorAttachment> colorAttachments) = 0;
    virtual void EndRenderPass() = 0;
};

} // namespace Quake::Renderer