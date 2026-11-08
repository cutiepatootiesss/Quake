#pragma once

#include <cstddef>
#include <memory>
#include <span>
#include <type_traits>
#include <vector>
#include <volk.h>

#include "../../SDL/SDL.hpp"
#include "VulkanDevice.hpp"
#include "VulkanSurface.hpp"
#include "VulkanSync.hpp"

namespace QuakeLauncher::Renderer
{

enum class SwapchainStatus
{
    Optimal,
    Suboptimal,
    OutOfDate,
};

struct AcquireResult
{
    SwapchainStatus status;
    std::uint32_t imageIndex;
};

class VulkanSwapchain
{
  public:
    explicit VulkanSwapchain(const VulkanDevice &device, const VulkanSurface &surface);

    [[nodiscard]] bool Recreate(const VulkanDevice &device, const VulkanSurface &surface, const SDL::Window &window);

    VulkanSwapchain(const VulkanSwapchain &) = delete;
    VulkanSwapchain &operator=(const VulkanSwapchain &) = delete;
    VulkanSwapchain(VulkanSwapchain &&) = delete;
    VulkanSwapchain &operator=(VulkanSwapchain &&) = delete;

    [[nodiscard]] AcquireResult AcquireNextImage(const VulkanSemaphore &imageAvailable) const;
    [[nodiscard]] SwapchainStatus Present(VkQueue queue, std::uint32_t imageIndex,
                                          const VulkanSemaphore &renderFinished) const;

    [[nodiscard]] VkSwapchainKHR GetSwapchain() const noexcept
    {
        return m_swapchain.get();
    }

    [[nodiscard]] VkFormat GetFormat() const noexcept
    {
        return m_format;
    }

    [[nodiscard]] VkExtent2D GetExtent() const noexcept
    {
        return m_extent;
    }

    [[nodiscard]] std::span<const VkImage> GetImages() const noexcept
    {
        return m_images;
    }

    [[nodiscard]] VkImageView GetImageView(std::size_t index) const
    {
        return m_imageViews.at(index).get();
    }

  private:
    struct SwapchainDeleter
    {
        explicit SwapchainDeleter(VkDevice owner) noexcept
            : device{owner}
        {
        }

        void operator()(VkSwapchainKHR swapchain) const noexcept
        {
            vkDestroySwapchainKHR(device, swapchain, nullptr);
        }

        VkDevice device;
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

    VkDevice m_device{VK_NULL_HANDLE};
    VkFormat m_format{VK_FORMAT_UNDEFINED};
    VkColorSpaceKHR m_colorSpace{VK_COLOR_SPACE_SRGB_NONLINEAR_KHR};
    VkExtent2D m_extent{};

    std::unique_ptr<std::remove_pointer_t<VkSwapchainKHR>, SwapchainDeleter> m_swapchain;
    std::vector<VkImage> m_images; // Owned by the swapchain
    std::vector<std::unique_ptr<std::remove_pointer_t<VkImageView>, ImageViewDeleter>> m_imageViews;
};

} // namespace QuakeLauncher::Renderer