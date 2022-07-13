#pragma once
#include "Pipeline.h"

#include <vulkan/vulkan.h>

#include "RenderPassVK.h"
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

        VkPipeline Get() { return m_Handle; }
        ShaderProgramVK* GetShaderProgram() { 
			switch (m_Type) {
			case PipelineBindPoint::Graphics:
				return static_cast<ShaderProgramVK*>(m_GraphicsDesc.ShaderProgram);
			case PipelineBindPoint::Compute:
				return static_cast<ShaderProgramVK*>(m_ComputeDesc.ShaderProgram);
			case PipelineBindPoint::Raytracing:
				return static_cast<ShaderProgramVK*>(m_RaytracingDesc.ShaderProgram);
			default:
				throw std::runtime_error("Unknown Bind Point!");
			}
		}

    private:
        VkPipeline m_Handle = VK_NULL_HANDLE;
        GraphicsDeviceVK* m_Device = nullptr;
    };
}
