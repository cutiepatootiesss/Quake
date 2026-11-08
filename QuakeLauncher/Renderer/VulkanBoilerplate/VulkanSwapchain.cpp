#include "VulkanSwapchain.hpp"

#include <algorithm>
#include <cstdint>
#include <limits>
#include <span>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>
#include <volk.h>

namespace QuakeLauncher::Renderer
{
namespace
{

[[nodiscard]] SwapchainStatus ToStatus(VkResult result, std::string_view call)
{
    if (result == VK_SUBOPTIMAL_KHR)
    {
        return SwapchainStatus::Suboptimal;
    }
    if (result == VK_ERROR_OUT_OF_DATE_KHR)
    {
        return SwapchainStatus::OutOfDate;
    }
    ThrowIfFailed(result, call);
    return SwapchainStatus::Optimal;
}

[[nodiscard]] VkSurfaceFormatKHR ChooseSurfaceFormat(std::span<const VkSurfaceFormatKHR> formats)
{
    const auto preferred{std::ranges::find_if(formats,
                                              [](const VkSurfaceFormatKHR &format)
                                              {
                                                  return format.format == VK_FORMAT_B8G8R8A8_SRGB &&
                                                         format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
                                              })};
    return preferred != formats.end() ? *preferred : formats.front();
}

[[nodiscard]] VkPresentModeKHR ChoosePresentMode(std::span<const VkPresentModeKHR> presentModes)
{
    const bool preferred{std::ranges::find(presentModes, VK_PRESENT_MODE_MAILBOX_KHR) != presentModes.end()};
    return preferred ? VK_PRESENT_MODE_MAILBOX_KHR : VK_PRESENT_MODE_FIFO_KHR;
}

[[nodiscard]] VkExtent2D ChooseExtent(const VkSurfaceCapabilitiesKHR &capabilities, VkExtent2D framebufferExtent)
{
    // This is the case on Wayland.
    constexpr std::uint32_t kUndefined{std::numeric_limits<std::uint32_t>::max()};
    if (capabilities.currentExtent.width != kUndefined)
    {
        return capabilities.currentExtent;
    }

    return VkExtent2D{
        std::clamp(framebufferExtent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width),
        std::clamp(framebufferExtent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height)};
}

[[nodiscard]] std::uint32_t ChooseImageCount(const VkSurfaceCapabilitiesKHR &capabilities) noexcept
{
    const std::uint32_t desired{capabilities.minImageCount + 1};
    return capabilities.maxImageCount == 0 ? desired : std::min(desired, capabilities.maxImageCount);
}

} // namespace

VulkanSwapchain::VulkanSwapchain(const VulkanDevice &device, const VulkanSurface &surface)
    : m_device{device.GetLogicalDevice()},
      m_swapchain{nullptr, SwapchainDeleter{m_device}}
{
    std::uint32_t formatCount{0};
    ThrowIfFailed(
        vkGetPhysicalDeviceSurfaceFormatsKHR(device.GetPhysicalDevice(), surface.GetSurface(), &formatCount, nullptr),
        "vkGetPhysicalDeviceSurfaceFormatsKHR");
    std::vector<VkSurfaceFormatKHR> formats(formatCount);
    ThrowIfFailed(vkGetPhysicalDeviceSurfaceFormatsKHR(device.GetPhysicalDevice(), surface.GetSurface(), &formatCount,
                                                       formats.data()),
                  "vkGetPhysicalDeviceSurfaceFormatsKHR");
    if (formats.empty())
    {
        throw std::runtime_error{"The surface does not support any formats"};
    }

    const VkSurfaceFormatKHR surfaceFormat{ChooseSurfaceFormat(formats)};
    m_format = surfaceFormat.format;
    m_colorSpace = surfaceFormat.colorSpace;
}

bool VulkanSwapchain::Recreate(const VulkanDevice &device, const VulkanSurface &surface, const SDL::Window &window)
{
    const VkPhysicalDevice physicalDevice{device.GetPhysicalDevice()};
    const VkDevice logicalDevice{device.GetLogicalDevice()};
    const VkSurfaceKHR vkSurface{surface.GetSurface()};

    VkSurfaceCapabilitiesKHR capabilities{};
    ThrowIfFailed(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, vkSurface, &capabilities),
                  "vkGetPhysicalDeviceSurfaceCapabilitiesKHR");

    std::uint32_t presentModeCount{0};
    ThrowIfFailed(vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, vkSurface, &presentModeCount, nullptr),
                  "vkGetPhysicalDeviceSurfacePresentModesKHR");
    std::vector<VkPresentModeKHR> presentModes(presentModeCount);
    ThrowIfFailed(
        vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, vkSurface, &presentModeCount, presentModes.data()),
        "vkGetPhysicalDeviceSurfacePresentModesKHR");

    if (presentModes.empty())
    {
        throw std::runtime_error{"The surface does not support any present modes"};
    }

    const auto [width, height]{window.GetSize()};
    const VkExtent2D extent{
        ChooseExtent(capabilities, VkExtent2D{static_cast<std::uint32_t>(width), static_cast<std::uint32_t>(height)})};
    if (extent.width == 0 || extent.height == 0)
    {
        return false; // The window is minimized since a swapchain obviously cannot be zero sized
    }

    // Nothing may still be using the old swapchain's images.
    device.WaitIdle();

    m_extent = extent;

    VkSwapchainCreateInfoKHR swapchainCreateInfo{};
    swapchainCreateInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    swapchainCreateInfo.surface = vkSurface;
    swapchainCreateInfo.minImageCount = ChooseImageCount(capabilities);
    swapchainCreateInfo.imageFormat = m_format;
    swapchainCreateInfo.imageColorSpace = m_colorSpace;
    swapchainCreateInfo.imageExtent = m_extent;
    swapchainCreateInfo.imageArrayLayers = 1;
    swapchainCreateInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    swapchainCreateInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    swapchainCreateInfo.preTransform = capabilities.currentTransform;
    swapchainCreateInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    swapchainCreateInfo.presentMode = ChoosePresentMode(presentModes);
    swapchainCreateInfo.clipped = VK_TRUE;
    swapchainCreateInfo.oldSwapchain = m_swapchain.get();

    VkSwapchainKHR swapchain{VK_NULL_HANDLE};
    ThrowIfFailed(vkCreateSwapchainKHR(logicalDevice, &swapchainCreateInfo, nullptr, &swapchain),
                  "vkCreateSwapchainKHR");

    // The views reference the old images, so they must be destroyed before the old swapchain is.
    m_imageViews.clear();
    m_swapchain.reset(swapchain); // Destroys the old swapchain.

    std::uint32_t imageCount{0};
    ThrowIfFailed(vkGetSwapchainImagesKHR(logicalDevice, swapchain, &imageCount, nullptr), "vkGetSwapchainImagesKHR");
    m_images.resize(imageCount);
    ThrowIfFailed(vkGetSwapchainImagesKHR(logicalDevice, swapchain, &imageCount, m_images.data()),
                  "vkGetSwapchainImagesKHR");

    m_imageViews.reserve(m_images.size());
    for (const VkImage image : m_images)
    {
        VkImageViewCreateInfo imageViewCreateInfo{};
        imageViewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        imageViewCreateInfo.image = image;
        imageViewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        imageViewCreateInfo.format = m_format;
        imageViewCreateInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        imageViewCreateInfo.subresourceRange.levelCount = 1;
        imageViewCreateInfo.subresourceRange.layerCount = 1;

        VkImageView imageView{VK_NULL_HANDLE};
        ThrowIfFailed(vkCreateImageView(logicalDevice, &imageViewCreateInfo, nullptr, &imageView), "vkCreateImageView");
        m_imageViews.emplace_back(imageView, ImageViewDeleter{logicalDevice});
    }

    return true;
}

AcquireResult VulkanSwapchain::AcquireNextImage(const VulkanSemaphore &imageAvailable) const
{
    std::uint32_t imageIndex{0};
    const VkResult result{vkAcquireNextImageKHR(m_device, m_swapchain.get(), std::numeric_limits<std::uint64_t>::max(),
                                                imageAvailable.GetSemaphore(), VK_NULL_HANDLE, &imageIndex)};
    return AcquireResult{ToStatus(result, "vkAcquireNextImageKHR"), imageIndex};
}

SwapchainStatus VulkanSwapchain::Present(VkQueue queue, std::uint32_t imageIndex,
                                         const VulkanSemaphore &renderFinished) const
{
    const VkSwapchainKHR swapchain{m_swapchain.get()};
    const VkSemaphore waitSemaphore{renderFinished.GetSemaphore()};

    VkPresentInfoKHR presentInfo{};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = &waitSemaphore;
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = &swapchain;
    presentInfo.pImageIndices = &imageIndex;

    return ToStatus(vkQueuePresentKHR(queue, &presentInfo), "vkQueuePresentKHR");
}

} // namespace QuakeLauncher::Renderer