#pragma once

#include <cstddef>
#include <cstdint>
#include <span>
#include <stdexcept>
#include <utility>
#include <volk.h>

#include "../../Quake/Renderer/Buffer.hpp"
#include "VulkanBoilerplate/VulkanBuffer.hpp"

namespace QuakeLauncher::Renderer
{

[[nodiscard]] constexpr VkBufferUsageFlags ToVulkan(Quake::Renderer::BufferUsage usage)
{
    switch (usage)
    {
    case Quake::Renderer::BufferUsage::Vertex:
        return VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
    case Quake::Renderer::BufferUsage::Index:
        return VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
    }
    throw std::invalid_argument{"Unknown BufferUsage"};
}

[[nodiscard]] constexpr VkIndexType ToVulkan(Quake::Renderer::IndexType indexType)
{
    switch (indexType)
    {
    case Quake::Renderer::IndexType::UInt16:
        return VK_INDEX_TYPE_UINT16;
    case Quake::Renderer::IndexType::UInt32:
        return VK_INDEX_TYPE_UINT32;
    }
    throw std::invalid_argument{"Unknown IndexType"};
}

class VulkanBufferImpl final : public Quake::Renderer::Buffer
{
  public:
    VulkanBufferImpl(const VulkanAllocator &allocator, std::uint64_t size, Quake::Renderer::BufferUsage usage,
                     const std::uint32_t &frameIndex)
        : m_usage{usage},
          m_frameIndex{frameIndex}
    {
        m_buffers.reserve(kFramesInFlight);
        for (std::uint32_t i{0}; i < kFramesInFlight; ++i)
        {
            m_buffers.emplace_back(allocator, size, ToVulkan(usage));
        }
    }

    /// Writes to every copy.
    void WriteAll(std::span<const std::byte> data) const
    {
        for (const VulkanBuffer &buffer : m_buffers)
        {
            buffer.Write(data, 0);
        }
    }

    [[nodiscard]] Quake::Renderer::BufferUsage GetUsage() const noexcept override
    {
        return m_usage;
    }

    [[nodiscard]] std::uint64_t GetSize() const noexcept override
    {
        return m_buffers.front().GetSize();
    }

    [[nodiscard]] VkBuffer GetBuffer() const noexcept
    {
        return m_buffers[m_frameIndex].GetBuffer();
    }

  private:
    void WriteBytes(std::span<const std::byte> data, std::uint64_t offset) override
    {
        m_buffers[m_frameIndex].Write(data, offset);
    }

    std::vector<VulkanBuffer> m_buffers;
    Quake::Renderer::BufferUsage m_usage;
    const std::uint32_t &m_frameIndex;
};

} // namespace QuakeLauncher::Renderer