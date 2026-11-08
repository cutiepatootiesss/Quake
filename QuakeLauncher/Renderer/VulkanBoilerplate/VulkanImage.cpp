#include "VulkanImage.hpp"

#include <stdexcept>
#include <volk.h>

namespace QuakeLauncher::Renderer
{

VulkanImage::VulkanImage(const VulkanDevice &device, const VulkanAllocator &allocator,
                         const VkImageCreateInfo &imageInfo, VkImageAspectFlags aspectMask)
    : m_extent{imageInfo.extent.width, imageInfo.extent.height},
      m_format{imageInfo.format},
      m_samples{imageInfo.samples},
      m_imageView{nullptr, ImageViewDeleter{device.GetLogicalDevice()}}
{
    VmaAllocationCreateInfo allocationInfo{};
    allocationInfo.usage = VMA_MEMORY_USAGE_AUTO;
    allocationInfo.flags = VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT;

    VkImage image{VK_NULL_HANDLE};
    VmaAllocation allocation{nullptr};
    ThrowIfFailed(vmaCreateImage(allocator.GetAllocator(), &imageInfo, &allocationInfo, &image, &allocation, nullptr),
                  "vmaCreateImage");
    m_image = decltype(m_image){image, ImageDeleter{allocator.GetAllocator(), allocation}};

    VkImageViewCreateInfo viewInfo{};
    viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewInfo.image = image;
    viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    viewInfo.format = m_format;
    viewInfo.subresourceRange.aspectMask = aspectMask;
    viewInfo.subresourceRange.levelCount = 1;
    viewInfo.subresourceRange.layerCount = 1;

    VkImageView imageView{VK_NULL_HANDLE};
    ThrowIfFailed(vkCreateImageView(device.GetLogicalDevice(), &viewInfo, nullptr, &imageView), "vkCreateImageView");
    m_imageView.reset(imageView);
}

VulkanImage VulkanImageBuilder::Build(const VulkanDevice &device, const VulkanAllocator &allocator) const
{
    if (m_extent.width == 0 || m_extent.height == 0)
    {
        throw std::logic_error{"An image cannot be zero-sized"};
    }
    if (m_format == VK_FORMAT_UNDEFINED)
    {
        throw std::logic_error{"An image needs a format"};
    }
    if (m_usage == 0)
    {
        throw std::logic_error{"An image needs a usage"};
    }

    VkImageCreateInfo imageInfo{};
    imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageInfo.imageType = VK_IMAGE_TYPE_2D;
    imageInfo.format = m_format;
    imageInfo.extent = VkExtent3D{m_extent.width, m_extent.height, 1};
    imageInfo.mipLevels = 1;
    imageInfo.arrayLayers = 1;
    imageInfo.samples = m_samples;
    imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    imageInfo.usage = m_usage;
    imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

    return VulkanImage{device, allocator, imageInfo, m_aspectMask};
}

} // namespace QuakeLauncher::Renderer