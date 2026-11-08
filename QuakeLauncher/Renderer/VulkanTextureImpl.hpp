#pragma once

#include <cstdint>
#include <optional>
#include <stdexcept>
#include <utility>
#include <volk.h>

#include "../../Quake/Renderer/Texture.hpp"
#include "VulkanBoilerplate/VulkanImage.hpp"

namespace QuakeLauncher::Renderer
{

[[nodiscard]] constexpr VkSampleCountFlagBits ToVulkan(Quake::Renderer::SampleCount sampleCount) noexcept
{
    return static_cast<VkSampleCountFlagBits>(sampleCount);
}

[[nodiscard]] constexpr Quake::Renderer::SampleCount ToSampleCount(VkSampleCountFlagBits samples) noexcept
{
    return static_cast<Quake::Renderer::SampleCount>(samples);
}

[[nodiscard]] constexpr VkFormat ToVulkan(Quake::Renderer::TextureFormat format)
{
    switch (format)
    {
    case Quake::Renderer::TextureFormat::RGBA8Unorm:
        return VK_FORMAT_R8G8B8A8_UNORM;
    case Quake::Renderer::TextureFormat::RGBA8Srgb:
        return VK_FORMAT_R8G8B8A8_SRGB;
    case Quake::Renderer::TextureFormat::BGRA8Unorm:
        return VK_FORMAT_B8G8R8A8_UNORM;
    case Quake::Renderer::TextureFormat::BGRA8Srgb:
        return VK_FORMAT_B8G8R8A8_SRGB;
    }
    throw std::invalid_argument{"Unknown TextureFormat"};
}

[[nodiscard]] constexpr Quake::Renderer::TextureFormat ToTextureFormat(VkFormat format)
{
    switch (format)
    {
    case VK_FORMAT_R8G8B8A8_UNORM:
        return Quake::Renderer::TextureFormat::RGBA8Unorm;
    case VK_FORMAT_R8G8B8A8_SRGB:
        return Quake::Renderer::TextureFormat::RGBA8Srgb;
    case VK_FORMAT_B8G8R8A8_UNORM:
        return Quake::Renderer::TextureFormat::BGRA8Unorm;
    case VK_FORMAT_B8G8R8A8_SRGB:
        return Quake::Renderer::TextureFormat::BGRA8Srgb;
    default:
        break;
    }
    throw std::invalid_argument{"VkFormat has no TextureFormat equivalent"};
}

class VulkanTextureImpl final : public Quake::Renderer::Texture
{
  public:
    VulkanTextureImpl(VulkanImage image, bool matchBackbuffer)
        : m_matchBackbuffer{matchBackbuffer}
    {
        Replace(std::move(image));
    }

    VulkanTextureImpl(VkImage image, VkImageView imageView, VkExtent2D extent, VkFormat format) noexcept
        : m_image{image},
          m_imageView{imageView},
          m_extent{extent},
          m_format{format}
    {
    }

    void Replace(VulkanImage image)
    {
        m_owned.reset();
        m_owned.emplace(std::move(image));
        m_image = m_owned->GetImage();
        m_imageView = m_owned->GetImageView();
        m_extent = m_owned->GetExtent();
        m_format = m_owned->GetFormat();
        m_samples = m_owned->GetSamples();
    }

    [[nodiscard]] std::uint32_t GetWidth() const noexcept override
    {
        return m_extent.width;
    }

    [[nodiscard]] std::uint32_t GetHeight() const noexcept override
    {
        return m_extent.height;
    }

    [[nodiscard]] Quake::Renderer::TextureFormat GetFormat() const override
    {
        return ToTextureFormat(m_format);
    }

    [[nodiscard]] Quake::Renderer::SampleCount GetSampleCount() const noexcept override
    {
        return ToSampleCount(m_samples);
    }

    [[nodiscard]] VkImage GetImage() const noexcept
    {
        return m_image;
    }

    [[nodiscard]] VkImageView GetImageView() const noexcept
    {
        return m_imageView;
    }

    [[nodiscard]] VkExtent2D GetExtent() const noexcept
    {
        return m_extent;
    }

    [[nodiscard]] bool MatchesBackbuffer() const noexcept
    {
        return m_matchBackbuffer;
    }

  private:
    std::optional<VulkanImage> m_owned;
    VkImage m_image{VK_NULL_HANDLE};
    VkImageView m_imageView{VK_NULL_HANDLE};
    VkExtent2D m_extent{};
    VkFormat m_format{VK_FORMAT_UNDEFINED};
    VkSampleCountFlagBits m_samples{VK_SAMPLE_COUNT_1_BIT};
    bool m_matchBackbuffer{false};
};

} // namespace QuakeLauncher::Renderer