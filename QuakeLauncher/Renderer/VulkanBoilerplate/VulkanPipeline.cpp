#include "VulkanPipeline.hpp"

#include <array>
#include <cstdint>
#include <span>
#include <stdexcept>
#include <utility>
#include <volk.h>

namespace QuakeLauncher::Renderer
{
namespace
{

[[nodiscard]] VkShaderModuleCreateInfo MakeShaderModuleInfo(std::span<const std::uint32_t> spirv) noexcept
{
    VkShaderModuleCreateInfo info{};
    info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    info.codeSize = spirv.size_bytes();
    info.pCode = spirv.data();
    return info;
}

[[nodiscard]] VkPipelineShaderStageCreateInfo MakeShaderStage(VkShaderStageFlagBits stage,
                                                              const VkShaderModuleCreateInfo &module) noexcept
{
    VkPipelineShaderStageCreateInfo info{};
    info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    info.pNext = &module;
    info.stage = stage;
    info.pName = "main";
    return info;
}

} // namespace

VulkanPipelineLayout::VulkanPipelineLayout(const VulkanDevice &device)
    : m_layout{nullptr, LayoutDeleter{device.GetLogicalDevice()}}
{
    constexpr VkPushConstantRange range{kPushConstantStages, 0, kPushConstantSize};

    VkPipelineLayoutCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    createInfo.pushConstantRangeCount = 1;
    createInfo.pPushConstantRanges = &range;

    VkPipelineLayout layout{VK_NULL_HANDLE};
    ThrowIfFailed(vkCreatePipelineLayout(device.GetLogicalDevice(), &createInfo, nullptr, &layout),
                  "vkCreatePipelineLayout");
    m_layout.reset(layout);
}

VulkanPipeline VulkanGraphicsPipelineBuilder::Build(const VulkanDevice &device) const
{
    if (m_vertexShader.empty() || m_fragmentShader.empty())
    {
        throw std::logic_error{"A graphics pipeline needs a vertex and a fragment shader"};
    }
    if (m_colorFormat == VK_FORMAT_UNDEFINED)
    {
        throw std::logic_error{"A graphics pipeline needs a color format"};
    }

    const VkShaderModuleCreateInfo vertexModule{MakeShaderModuleInfo(m_vertexShader)};
    const VkShaderModuleCreateInfo fragmentModule{MakeShaderModuleInfo(m_fragmentShader)};
    const std::array stages{MakeShaderStage(VK_SHADER_STAGE_VERTEX_BIT, vertexModule),
                            MakeShaderStage(VK_SHADER_STAGE_FRAGMENT_BIT, fragmentModule)};

    VkVertexInputBindingDescription vertexBinding{};
    vertexBinding.binding = 0;
    vertexBinding.stride = m_vertexStride;
    vertexBinding.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

    VkPipelineVertexInputStateCreateInfo vertexInput{};
    vertexInput.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInput.vertexBindingDescriptionCount = m_vertexAttributes.empty() ? 0U : 1U;
    vertexInput.pVertexBindingDescriptions = &vertexBinding;
    vertexInput.vertexAttributeDescriptionCount = static_cast<std::uint32_t>(m_vertexAttributes.size());
    vertexInput.pVertexAttributeDescriptions = m_vertexAttributes.data();

    VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
    inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssembly.topology = m_topology;

    VkPipelineViewportStateCreateInfo viewportState{};
    viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportState.viewportCount = 1;
    viewportState.scissorCount = 1;

    VkPipelineRasterizationStateCreateInfo rasterization{};
    rasterization.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterization.polygonMode = VK_POLYGON_MODE_FILL;
    rasterization.cullMode = m_cullMode;
    rasterization.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    rasterization.lineWidth = 1.0F;

    VkPipelineMultisampleStateCreateInfo multisample{};
    multisample.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisample.rasterizationSamples = m_samples;

    VkPipelineColorBlendAttachmentState blendAttachment{};
    blendAttachment.colorWriteMask =
        VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;

    VkPipelineColorBlendStateCreateInfo colorBlend{};
    colorBlend.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlend.attachmentCount = 1;
    colorBlend.pAttachments = &blendAttachment;

    constexpr std::array dynamicStates{VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};
    VkPipelineDynamicStateCreateInfo dynamicState{};
    dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamicState.dynamicStateCount = static_cast<std::uint32_t>(dynamicStates.size());
    dynamicState.pDynamicStates = dynamicStates.data();

    VkPipelineRenderingCreateInfo rendering{};
    rendering.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO;
    rendering.colorAttachmentCount = 1;
    rendering.pColorAttachmentFormats = &m_colorFormat;

    VulkanPipelineLayout layout{device};

    VkGraphicsPipelineCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    createInfo.pNext = &rendering;
    createInfo.stageCount = static_cast<std::uint32_t>(stages.size());
    createInfo.pStages = stages.data();
    createInfo.pVertexInputState = &vertexInput;
    createInfo.pInputAssemblyState = &inputAssembly;
    createInfo.pViewportState = &viewportState;
    createInfo.pRasterizationState = &rasterization;
    createInfo.pMultisampleState = &multisample;
    createInfo.pColorBlendState = &colorBlend;
    createInfo.pDynamicState = &dynamicState;
    createInfo.layout = layout.GetPipelineLayout();

    VkPipeline pipeline{VK_NULL_HANDLE};
    ThrowIfFailed(
        vkCreateGraphicsPipelines(device.GetLogicalDevice(), VK_NULL_HANDLE, 1, &createInfo, nullptr, &pipeline),
        "vkCreateGraphicsPipelines");
    return VulkanPipeline{device, std::move(layout), pipeline};
}

} // namespace QuakeLauncher::Renderer