#pragma once
#include "Pipeline.h"

#include <vulkan/vulkan.h>

#include "RenderPass.h"
#include "ShaderVK.h"

namespace Vulkan
{
    class GraphicsDeviceVK;
    class PipelineVK : public Pipeline
    {
    public:
        PipelineVK(GraphicsDeviceVK* device, const GraphicsPipelineDesc& createInfo, RenderPass* renderPass);
        PipelineVK(GraphicsDeviceVK* device, const ComputePipelineDesc& createInfo);
        PipelineVK(GraphicsDeviceVK* device, const RaytracingPipelineDesc& createInfo);
        ~PipelineVK();

        VkPipeline Get() { return m_Pipeline; }
        ShaderProgramVK* GetProgram() { return m_Program; }

    private:
        VkPipeline m_Pipeline;
        GraphicsDeviceVK* m_Device;

        ShaderProgramVK* m_Program;
    };
}
