#include "VulkanBuffer.hpp"

#include <stdexcept>
#include <volk.h>

namespace QuakeLauncher::Renderer
{

VulkanBuffer::VulkanBuffer(const VulkanAllocator &allocator, VkDeviceSize size, VkBufferUsageFlags usage)
    : m_size{size}
{
    VkBufferCreateInfo bufferInfo{};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = size;
    bufferInfo.usage = usage;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    VmaAllocationCreateInfo allocationInfo{};
    allocationInfo.usage = VMA_MEMORY_USAGE_AUTO;
    allocationInfo.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;

    VkBuffer buffer{VK_NULL_HANDLE};
    VmaAllocation allocation{nullptr};
    ThrowIfFailed(
        vmaCreateBuffer(allocator.GetAllocator(), &bufferInfo, &allocationInfo, &buffer, &allocation, nullptr),
        "vmaCreateBuffer");
    m_buffer = decltype(m_buffer){buffer, BufferDeleter{allocator.GetAllocator(), allocation}};
}

void VulkanBuffer::Write(std::span<const std::byte> data, VkDeviceSize offset) const
{
    if (data.empty())
    {
        return;
    }
    if (offset > m_size || data.size() > m_size - offset)
    {
        throw std::out_of_range{"Buffer write is out of bounds"};
    }

    const BufferDeleter &memory{m_buffer.get_deleter()};
    ThrowIfFailed(vmaCopyMemoryToAllocation(memory.allocator, data.data(), memory.allocation, offset, data.size()),
                  "vmaCopyMemoryToAllocation");
}

} // namespace QuakeLauncher::Renderer