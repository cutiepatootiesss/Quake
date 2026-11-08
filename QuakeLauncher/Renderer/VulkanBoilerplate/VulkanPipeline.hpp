#pragma once

#include <cstdint>
#include <memory>
#include <span>
#include <type_traits>
#include <utility>
#include <volk.h>

#include "VulkanDevice.hpp"

namespace QuakeLauncher::Renderer
{

inline constexpr std::uint32_t kPushConstantSize{256}; // Minimum guarantee.
inline constexpr VkShaderStageFlags kPushConstantStages{VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT};

class VulkanPipelineLayout
{
  public:
    explicit VulkanPipelineLayout(const VulkanDevice &device);
    ~VulkanPipelineLayout() = default;

    VulkanPipelineLayout(const VulkanPipelineLayout &) = delete;
    VulkanPipelineLayout &operator=(const VulkanPipelineLayout &) = delete;
    VulkanPipelineLayout(VulkanPipelineLayout &&) noexcept = default;
    VulkanPipelineLayout &operator=(VulkanPipelineLayout &&) noexcept = default;

    [[nodiscard]] VkPipelineLayout GetPipelineLayout() const noexcept
    {
        return m_layout.get();
    }

  private:
    struct LayoutDeleter
    {
        explicit LayoutDeleter(VkDevice owner) noexcept
            : device{owner}
        {
        }

        void operator()(VkPipelineLayout layout) const noexcept
        {
            vkDestroyPipelineLayout(device, layout, nullptr);
        }

        VkDevice device;
    };

    std::unique_ptr<std::remove_pointer_t<VkPipelineLayout>, LayoutDeleter> m_layout;
};

class VulkanPipeline
{
  public:
    VulkanPipeline(const VulkanDevice &device, VulkanPipelineLayout layout, VkPipeline pipeline) noexcept
        : m_layout{std::move(layout)},
          m_pipeline{pipeline, PipelineDeleter{device.GetLogicalDevice()}}
    {
    }

    ~VulkanPipeline() = default;

    VulkanPipeline(const VulkanPipeline &) = delete;
    VulkanPipeline &operator=(const VulkanPipeline &) = delete;
    VulkanPipeline(VulkanPipeline &&) noexcept = default;
    VulkanPipeline &operator=(VulkanPipeline &&) noexcept = default;

    [[nodiscard]] VkPipeline GetPipeline() const noexcept
    {
        return m_pipeline.get();
    }

    [[nodiscard]] VkPipelineLayout GetPipelineLayout() const noexcept
    {
        return m_layout.GetPipelineLayout();
    }

  private:
    struct PipelineDeleter
    {
        explicit PipelineDeleter(VkDevice owner) noexcept
            : device{owner}
        {
        }

        void operator()(VkPipeline pipeline) const noexcept
        {
            vkDestroyPipeline(device, pipeline, nullptr);
        }

        VkDevice device;
    };

    VulkanPipelineLayout m_layout;
    std::unique_ptr<std::remove_pointer_t<VkPipeline>, PipelineDeleter> m_pipeline;
};

class VulkanGraphicsPipelineBuilder
{
  public:
    VulkanGraphicsPipelineBuilder &SetVertexShader(std::span<const std::uint32_t> spirv) noexcept
    {
        m_vertexShader = spirv;
        return *this;
    }

    VulkanGraphicsPipelineBuilder &SetFragmentShader(std::span<const std::uint32_t> spirv) noexcept
    {
        m_fragmentShader = spirv;
        return *this;
    }

    VulkanGraphicsPipelineBuilder &SetTopology(VkPrimitiveTopology topology) noexcept
    {
        m_topology = topology;
        return *this;
    }

    VulkanGraphicsPipelineBuilder &SetCullMode(VkCullModeFlags cullMode) noexcept
    {
        m_cullMode = cullMode;
        return *this;
    }

    VulkanGraphicsPipelineBuilder &SetColorFormat(VkFormat format) noexcept
    {
        m_colorFormat = format;
        return *this;
    }

    VulkanGraphicsPipelineBuilder &SetSamples(VkSampleCountFlagBits samples) noexcept
    {
        m_samples = samples;
        return *this;
    }

    VulkanGraphicsPipelineBuilder &SetVertexInput(
        std::uint32_t stride,
        std::span<const VkVertexInputAttributeDescription> attributes) noexcept
    {
        m_vertexStride = stride;
        m_vertexAttributes = attributes;
        return *this;
    }

    [[nodiscard]] VulkanPipeline Build(const VulkanDevice &device) const;

  private:
    std::span<const std::uint32_t> m_vertexShader;
    std::span<const std::uint32_t> m_fragmentShader;
    VkPrimitiveTopology m_topology{VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST};
    VkCullModeFlags m_cullMode{VK_CULL_MODE_BACK_BIT};
    VkFormat m_colorFormat{VK_FORMAT_UNDEFINED};
    VkSampleCountFlagBits m_samples{VK_SAMPLE_COUNT_1_BIT};
    std::uint32_t m_vertexStride{0};
    std::span<const VkVertexInputAttributeDescription> m_vertexAttributes;
};

} // namespace QuakeLauncher::Renderer