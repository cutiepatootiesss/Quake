#pragma once

#include <cstdint>
#include <stdexcept>

namespace Quake::Renderer
{

enum class SampleCount : std::uint32_t
{
    X1 = 1,
    X2 = 2,
    X4 = 4,
    X8 = 8,
};

enum class TextureFormat
{
    RGBA8Unorm,
    RGBA8Srgb,
    BGRA8Unorm,
    BGRA8Srgb,
};

class Texture
{
  public:
    virtual ~Texture() = default;

    Texture(const Texture &) = delete;
    Texture &operator=(const Texture &) = delete;
    Texture(Texture &&) = delete;
    Texture &operator=(Texture &&) = delete;

    [[nodiscard]] virtual std::uint32_t GetWidth() const noexcept = 0;
    [[nodiscard]] virtual std::uint32_t GetHeight() const noexcept = 0;
    [[nodiscard]] virtual SampleCount GetSampleCount() const noexcept = 0;
    [[nodiscard]] virtual TextureFormat GetFormat() const = 0;

  protected:
    Texture() = default;
};

class TextureDescriptor
{
  public:
    constexpr TextureDescriptor(std::uint32_t width, std::uint32_t height, TextureFormat format,
                                SampleCount sampleCount, bool matchBackbuffer) noexcept
        : m_width{width},
          m_height{height},
          m_format{format},
          m_sampleCount{sampleCount},
          m_matchBackbuffer{matchBackbuffer}
    {
    }

    [[nodiscard]] constexpr std::uint32_t GetWidth() const noexcept
    {
        return m_width;
    }

    [[nodiscard]] constexpr std::uint32_t GetHeight() const noexcept
    {
        return m_height;
    }

    [[nodiscard]] constexpr TextureFormat GetFormat() const noexcept
    {
        return m_format;
    }

    [[nodiscard]] constexpr SampleCount GetSampleCount() const noexcept
    {
        return m_sampleCount;
    }

    [[nodiscard]] constexpr bool MatchesBackbuffer() const noexcept
    {
        return m_matchBackbuffer;
    }

  private:
    std::uint32_t m_width;
    std::uint32_t m_height;
    TextureFormat m_format;
    SampleCount m_sampleCount;
    bool m_matchBackbuffer;
};

class TextureBuilder
{
  public:
    constexpr TextureBuilder &SetSize(std::uint32_t width, std::uint32_t height) noexcept
    {
        m_width = width;
        m_height = height;
        return *this;
    }

    constexpr TextureBuilder &SetFormat(TextureFormat format) noexcept
    {
        m_format = format;
        return *this;
    }

    /// Anything above X1 creates a multisampled texture. It can't be sampled directly
    /// and has to be resolved into a single sample texture (see ColorAttachmentBuilder::SetResolveTarget)
    constexpr TextureBuilder &SetSampleCount(SampleCount sampleCount) noexcept
    {
        m_sampleCount = sampleCount;
        return *this;
    }

    /// The texture always has the backbuffer's size and is recreated when it changes.
    /// Overrides SetSize.
    constexpr TextureBuilder &SetMatchBackbuffer(bool match = true) noexcept
    {
        m_matchBackbuffer = match;
        return *this;
    }

    [[nodiscard]] TextureDescriptor Build() const
    {
        if (!m_matchBackbuffer && (m_width == 0 || m_height == 0))
        {
            throw std::logic_error{"A texture needs a non-zero size"};
        }
        return TextureDescriptor{m_width, m_height, m_format, m_sampleCount, m_matchBackbuffer};
    }

  private:
    std::uint32_t m_width{0};
    std::uint32_t m_height{0};
    TextureFormat m_format{TextureFormat::BGRA8Unorm};
    SampleCount m_sampleCount{SampleCount::X1};
    bool m_matchBackbuffer{false};
};

} // namespace Quake::Renderer