#pragma once

#include <cstddef>
#include <cstdint>
#include <span>
#include <type_traits>

#include "Buffer.hpp"
#include "Pipeline.hpp"

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
    constexpr ColorAttachment(Texture *target, Texture *resolveTarget, LoadOp loadOp, StoreOp storeOp,
                              Color clearColor) noexcept
        : m_target{target},
          m_resolveTarget{resolveTarget},
          m_loadOp{loadOp},
          m_storeOp{storeOp},
          m_clearColor{clearColor}
    {
    }

    [[nodiscard]] constexpr Texture *GetTarget() const noexcept
    {
        return m_target;
    }

    [[nodiscard]] constexpr Texture *GetResolveTarget() const noexcept
    {
        return m_resolveTarget;
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
    Texture *m_target;
    Texture *m_resolveTarget;

    LoadOp m_loadOp;
    StoreOp m_storeOp;
    Color m_clearColor;
};

class ColorAttachmentBuilder
{
  public:
    constexpr ColorAttachmentBuilder &SetTarget(Texture &target) noexcept
    {
        m_target = &target;
        return *this;
    }

    constexpr ColorAttachmentBuilder &SetResolveTarget(Texture &resolveTarget) noexcept
    {
        m_resolveTarget = &resolveTarget;
        return *this;
    }

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

    [[nodiscard]] ColorAttachment Build() const
    {
        if (m_resolveTarget != nullptr)
        {
            if (m_target == nullptr)
            {
                throw std::logic_error{"A resolve target needs an explicit multisampled target"};
            }
            if (m_target->GetSampleCount() == SampleCount::X1)
            {
                throw std::logic_error{"Resolve source must be multisampled"};
            }
            if (m_resolveTarget->GetSampleCount() != SampleCount::X1)
            {
                throw std::logic_error{"Resolve target must be single-sample"};
            }
            if (m_target->GetFormat() != m_resolveTarget->GetFormat() ||
                m_target->GetWidth() != m_resolveTarget->GetWidth() ||
                m_target->GetHeight() != m_resolveTarget->GetHeight())
            {
                throw std::logic_error{"Resolve source and target must match in size and format"};
            }
        }
        return ColorAttachment{m_target, m_resolveTarget, m_loadOp, m_storeOp, m_clearColor};
    }

  private:
    Texture *m_target{nullptr};
    Texture *m_resolveTarget{nullptr};

    LoadOp m_loadOp{LoadOp::Clear};
    StoreOp m_storeOp{StoreOp::Store};
    Color m_clearColor{};
};

/// Maximum size of the data passed to CommandList::PushConstants.
inline constexpr std::size_t kMaxPushConstantSize{256};

class CommandList
{
  public:
    virtual ~CommandList() = default;

    virtual void BeginRenderPass(const ColorAttachment &colorAttachment) = 0;
    virtual void SetPipeline(const Pipeline &pipeline) = 0;
    virtual void SetVertexBuffer(const Buffer &buffer) = 0;
    virtual void SetIndexBuffer(const Buffer &buffer, IndexType indexType) = 0;
    virtual void DrawIndexed(std::uint32_t indexCount) = 0;
    virtual void Draw(std::uint32_t vertexCount) = 0;
    virtual void EndRenderPass() = 0;

    template <typename T>
    void PushConstants(const T &value)
    {
        static_assert(std::is_trivially_copyable_v<T>);
        static_assert(sizeof(T) <= kMaxPushConstantSize);
        static_assert(sizeof(T) % 4 == 0);
        PushConstantsBytes(std::as_bytes(std::span{&value, 1}));
    }

  private:
    virtual void PushConstantsBytes(std::span<const std::byte> data) = 0;
};

} // namespace Quake::Renderer