#pragma once

#include <memory>
#include <type_traits>
#include <volk.h>

#include <vk_mem_alloc.h>

#include "VulkanDevice.hpp"
#include "VulkanInstance.hpp"

namespace QuakeLauncher::Renderer
{

class VulkanAllocator
{
  public:
    explicit VulkanAllocator(const VulkanInstance &instance, const VulkanDevice &device);
    ~VulkanAllocator() = default;

    VulkanAllocator(const VulkanAllocator &) = delete;
    VulkanAllocator &operator=(const VulkanAllocator &) = delete;
    VulkanAllocator(VulkanAllocator &&) noexcept = default;
    VulkanAllocator &operator=(VulkanAllocator &&) noexcept = default;

    [[nodiscard]] VmaAllocator GetAllocator() const noexcept
    {
        return m_allocator.get();
    }

  private:
    struct AllocatorDeleter
    {
        void operator()(VmaAllocator allocator) const noexcept
        {
            vmaDestroyAllocator(allocator);
        }
    };

    std::unique_ptr<std::remove_pointer_t<VmaAllocator>, AllocatorDeleter> m_allocator;
};

} // namespace QuakeLauncher::Renderer