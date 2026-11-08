#pragma once

#include <cstddef>
#include <cstdint>
#include <ranges>
#include <span>
#include <stdexcept>
#include <type_traits>

namespace Quake::Renderer
{

enum class BufferUsage
{
    Vertex,
    Index,
};

enum class IndexType
{
    UInt16,
    UInt32,
};

class Buffer
{
  public:
    virtual ~Buffer() = default;

    Buffer(const Buffer &) = delete;
    Buffer &operator=(const Buffer &) = delete;
    Buffer(Buffer &&) = delete;
    Buffer &operator=(Buffer &&) = delete;

    [[nodiscard]] virtual std::uint64_t GetSize() const noexcept = 0;
    [[nodiscard]] virtual BufferUsage GetUsage() const noexcept = 0;

    /// Only call this between BeginFrame() and EndFrame()!!!
    template <std::ranges::contiguous_range Range>
    void Write(const Range &data, std::uint64_t offset = 0)
    {
        static_assert(std::is_trivially_copyable_v<std::ranges::range_value_t<Range>>);
        WriteBytes(std::as_bytes(std::span{data}), offset);
    }

  protected:
    Buffer() = default;

  private:
    virtual void WriteBytes(std::span<const std::byte> data, std::uint64_t offset) = 0;
};

class BufferDescriptor
{
  public:
    constexpr BufferDescriptor(std::uint64_t size, BufferUsage usage, std::span<const std::byte> initialData) noexcept
        : m_size{size},
          m_usage{usage},
          m_initialData{initialData}
    {
    }

    [[nodiscard]] constexpr std::uint64_t GetSize() const noexcept
    {
        return m_size;
    }

    [[nodiscard]] constexpr BufferUsage GetUsage() const noexcept
    {
        return m_usage;
    }

    [[nodiscard]] constexpr std::span<const std::byte> GetInitialData() const noexcept
    {
        return m_initialData;
    }

  private:
    std::uint64_t m_size;
    BufferUsage m_usage;
    std::span<const std::byte> m_initialData;
};

class BufferBuilder
{
  public:
    constexpr BufferBuilder &SetUsage(BufferUsage usage) noexcept
    {
        m_usage = usage;
        return *this;
    }

    /// Ignored if SetData is used.
    constexpr BufferBuilder &SetSize(std::uint64_t size) noexcept
    {
        m_size = size;
        return *this;
    }

    /// Initial contents, which also define the size.
    /// Not copied until CreateBuffer.
    template <std::ranges::contiguous_range Range>
    BufferBuilder &SetData(const Range &data) noexcept
    {
        static_assert(std::is_trivially_copyable_v<std::ranges::range_value_t<Range>>);
        m_data = std::as_bytes(std::span{data});
        return *this;
    }

    [[nodiscard]] BufferDescriptor Build() const
    {
        const std::uint64_t size{m_data.empty() ? m_size : m_data.size()};
        if (size == 0)
        {
            throw std::logic_error{"A buffer needs a non-zero size"};
        }
        return BufferDescriptor{size, m_usage, m_data};
    }

  private:
    BufferUsage m_usage{BufferUsage::Vertex};
    std::uint64_t m_size{0};
    std::span<const std::byte> m_data;
};

} // namespace Quake::Renderer