#pragma once

#include <utility>
#include <volk.h>

#include "../../Quake/Renderer/Pipeline.hpp"
#include "VulkanBoilerplate/VulkanPipeline.hpp"

namespace QuakeLauncher::Renderer
{

class VulkanPipelineImpl final : public Quake::Renderer::Pipeline
{
  public:
    explicit VulkanPipelineImpl(VulkanPipeline pipeline) noexcept
        : m_pipeline{std::move(pipeline)}
    {
    }

    [[nodiscard]] VkPipeline GetPipeline() const noexcept
    {
        return m_pipeline.GetPipeline();
    }

    [[nodiscard]] VkPipelineLayout GetPipelineLayout() const noexcept
    {
        return m_pipeline.GetPipelineLayout();
    }

  private:
    VulkanPipeline m_pipeline;
};

} // namespace QuakeLauncher::Renderer