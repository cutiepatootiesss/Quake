#pragma once

#include <cstddef>
#include <memory>
#include <span>
#include <type_traits>
#include <volk.h>

#include <vk_mem_alloc.h>

#include "VulkanAllocator.hpp"

namespace QuakeLauncher::Renderer
{

class VulkanBuffer
{
  public:
    VulkanBuffer(const VulkanAllocator &allocator, VkDeviceSize size, VkBufferUsageFlags usage);
    ~VulkanBuffer() = default;

    VulkanBuffer(const VulkanBuffer &) = delete;
    VulkanBuffer &operator=(const VulkanBuffer &) = delete;
    VulkanBuffer(VulkanBuffer &&) noexcept = default;
    VulkanBuffer &operator=(VulkanBuffer &&) noexcept = default;

    void Write(std::span<const std::byte> data, VkDeviceSize offset) const;

    [[nodiscard]] VkBuffer GetBuffer() const noexcept
    {
        return m_buffer.get();
    }

    [[nodiscard]] VkDeviceSize GetSize() const noexcept
    {
        return m_size;
    }

  private:
    struct BufferDeleter
    {
        BufferDeleter() noexcept
            : allocator{nullptr},
              allocation{nullptr}
        {
        }

        BufferDeleter(VmaAllocator owner, VmaAllocation memory) noexcept
            : allocator{owner},
              allocation{memory}
        {
        }

        void operator()(VkBuffer buffer) const noexcept
        {
            vmaDestroyBuffer(allocator, buffer, allocation);
        }

        VmaAllocator allocator;
        VmaAllocation allocation;
    };

    VkDeviceSize m_size{0};
    std::unique_ptr<std::remove_pointer_t<VkBuffer>, BufferDeleter> m_buffer;
};

} // namespace QuakeLauncher::Renderer