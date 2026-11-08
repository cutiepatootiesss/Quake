#pragma once

#include <memory>
#include <type_traits>
#include <volk.h>

#include <vk_mem_alloc.h>

#include "VulkanAllocator.hpp"
#include "VulkanDevice.hpp"

namespace QuakeLauncher::Renderer
{

class VulkanImage
{
  public:
    VulkanImage(const VulkanDevice &device, const VulkanAllocator &allocator, const VkImageCreateInfo &imageInfo,
                VkImageAspectFlags aspectMask);
    ~VulkanImage() = default;

    VulkanImage(const VulkanImage &) = delete;
    VulkanImage &operator=(const VulkanImage &) = delete;
    VulkanImage(VulkanImage &&) noexcept = default;
    VulkanImage &operator=(VulkanImage &&) noexcept = default;

    [[nodiscard]] VkImage GetImage() const noexcept
    {
        return m_image.get();
    }

    [[nodiscard]] VkImageView GetImageView() const noexcept
    {
        return m_imageView.get();
    }

    [[nodiscard]] VkExtent2D GetExtent() const noexcept
    {
        return m_extent;
    }

    [[nodiscard]] VkFormat GetFormat() const noexcept
    {
        return m_format;
    }

    [[nodiscard]] VkSampleCountFlagBits GetSamples() const noexcept
    {
        return m_samples;
    }

  private:
    struct ImageDeleter
    {
        ImageDeleter() noexcept
            : allocator{nullptr},
              allocation{nullptr}
        {
        }

        ImageDeleter(VmaAllocator owner, VmaAllocation memory) noexcept
            : allocator{owner},
              allocation{memory}
        {
        }

        void operator()(VkImage image) const noexcept
        {
            vmaDestroyImage(allocator, image, allocation);
        }

        VmaAllocator allocator;
        VmaAllocation allocation;
    };

    struct ImageViewDeleter
    {
        explicit ImageViewDeleter(VkDevice owner) noexcept
            : device{owner}
        {
        }

        void operator()(VkImageView imageView) const noexcept
        {
            vkDestroyImageView(device, imageView, nullptr);
        }

        VkDevice device;
    };

    VkExtent2D m_extent{};
    VkFormat m_format{VK_FORMAT_UNDEFINED};
    VkSampleCountFlagBits m_samples{VK_SAMPLE_COUNT_1_BIT};

    std::unique_ptr<std::remove_pointer_t<VkImage>, ImageDeleter> m_image;
    std::unique_ptr<std::remove_pointer_t<VkImageView>, ImageViewDeleter> m_imageView;
};

class VulkanImageBuilder
{
  public:
    VulkanImageBuilder &SetExtent(VkExtent2D extent) noexcept
    {
        m_extent = extent;
        return *this;
    }

    VulkanImageBuilder &SetFormat(VkFormat format) noexcept
    {
        m_format = format;
        return *this;
    }

    VulkanImageBuilder &SetSamples(VkSampleCountFlagBits samples) noexcept
    {
        m_samples = samples;
        return *this;
    }

    VulkanImageBuilder &SetUsage(VkImageUsageFlags usage) noexcept
    {
        m_usage = usage;
        return *this;
    }

    VulkanImageBuilder &SetAspectMask(VkImageAspectFlags aspectMask) noexcept
    {
        m_aspectMask = aspectMask;
        return *this;
    }

    [[nodiscard]] VulkanImage Build(const VulkanDevice &device, const VulkanAllocator &allocator) const;

  private:
    VkExtent2D m_extent{};
    VkFormat m_format{VK_FORMAT_UNDEFINED};
    VkSampleCountFlagBits m_samples{VK_SAMPLE_COUNT_1_BIT};
    VkImageUsageFlags m_usage{0};
    VkImageAspectFlags m_aspectMask{VK_IMAGE_ASPECT_COLOR_BIT};
};

} // namespace QuakeLauncher::Renderer