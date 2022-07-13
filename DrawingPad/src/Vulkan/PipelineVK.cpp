#include "pwpch.h"
#include "PipelineVK.h"

#include "GraphicsDeviceVK.h"

namespace Vulkan
{
	PipelineVK::PipelineVK(GraphicsDeviceVK* device, const GraphicsPipelineDesc& desc, RenderPass* renderPass)
		: Pipeline(desc), m_Device(device), m_Handle(VK_NULL_HANDLE)
	{
		VkRenderPass renderpass = ((RenderPassVK*)renderPass)->Get();

		std::vector<VkVertexInputBindingDescription> bindingDesc;
		std::vector<VkVertexInputAttributeDescription> attribDesc;
		attribDesc.resize(desc.InputLayout.NumElements);
		uint32_t bindingSize = 0;
		// Pull vertex input and binding data from layout
		std::vector<int> bindingMap;
		bindingMap.resize(desc.InputLayout.NumElements, -1);
		for (uint32_t i = 0; i < desc.InputLayout.NumElements; i++)
		{
			auto& elem = desc.InputLayout.Elements[i];
			auto& bindIdx = bindingMap[elem.BufferSlot];
			if (bindIdx < 0)
			{
				bindIdx = bindingSize++;
				bindingDesc.push_back({ elem.BufferSlot, elem.Stride, VK_VERTEX_INPUT_RATE_VERTEX });
			}

			attribDesc[i].binding = elem.BufferSlot;
			attribDesc[i].location = elem.InputIndex;
			attribDesc[i].format = UtilsVK::AttribFormatToVk(elem.Type, elem.NumComponents, elem.Normalized);
			attribDesc[i].offset = elem.Offset;
		}

		VkPipelineVertexInputStateCreateInfo vertexInputState = {};
		vertexInputState.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
		vertexInputState.vertexBindingDescriptionCount = static_cast<uint32_t>(bindingDesc.size());
		vertexInputState.pVertexBindingDescriptions = bindingDesc.data();
		vertexInputState.vertexAttributeDescriptionCount = static_cast<uint32_t>(attribDesc.size());
		vertexInputState.pVertexAttributeDescriptions = attribDesc.data();

		// Potentially Modify
		VkPipelineInputAssemblyStateCreateInfo inputAssemblyState = {};
		inputAssemblyState.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
		inputAssemblyState.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
		//inputAssemblyState.primitiveRestartEnable = VK_FALSE;

		VkPipelineViewportStateCreateInfo viewportState = {};
		viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
		viewportState.viewportCount = desc.NumViewports;
		viewportState.scissorCount = viewportState.viewportCount;

		// Potentially Modify
		VkPipelineRasterizationStateCreateInfo rasterizationState = {};
		rasterizationState.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
		rasterizationState.depthClampEnable = VK_FALSE;
		rasterizationState.rasterizerDiscardEnable = VK_FALSE;
		rasterizationState.polygonMode = VK_POLYGON_MODE_FILL;
		rasterizationState.lineWidth = 1.0f;
		rasterizationState.cullMode = VK_CULL_MODE_NONE;
		rasterizationState.frontFace = (VkFrontFace)desc.Face;
		rasterizationState.depthBiasEnable = VK_FALSE;
		rasterizationState.depthBiasConstantFactor = 0.0f;
		rasterizationState.depthBiasClamp = 0.0f;
		rasterizationState.depthBiasSlopeFactor = 0.0f;

		VkPipelineMultisampleStateCreateInfo multisampleState = {};
		multisampleState.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
		multisampleState.rasterizationSamples = (desc.MSAASamples != 0) ? (VkSampleCountFlagBits)desc.MSAASamples : VK_SAMPLE_COUNT_1_BIT;

		VkPipelineColorBlendAttachmentState colorBlendAttachment = {};
		colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
		colorBlendAttachment.blendEnable = VK_TRUE;
		colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
		colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
		colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
		colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
		colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
		colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;

		// Potentially Modify
		VkPipelineColorBlendStateCreateInfo colorBlendState = {};
		colorBlendState.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
		colorBlendState.logicOpEnable = VK_FALSE;
		colorBlendState.logicOp = VK_LOGIC_OP_COPY;
		colorBlendState.attachmentCount = 1;
		colorBlendState.pAttachments = &colorBlendAttachment;
		colorBlendState.blendConstants[0] = 1.f;
		colorBlendState.blendConstants[1] = 1.f;
		colorBlendState.blendConstants[2] = 1.f;
		colorBlendState.blendConstants[3] = 1.f;

		VkPipelineDepthStencilStateCreateInfo depthStencilState = {};
		depthStencilState.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
		if (desc.DepthFormat != TextureFormat::None)
		{
			depthStencilState.depthTestEnable = VK_TRUE;
			depthStencilState.depthWriteEnable = VK_TRUE;
			depthStencilState.depthCompareOp = VK_COMPARE_OP_LESS;
			depthStencilState.depthBoundsTestEnable = VK_FALSE;
			depthStencilState.minDepthBounds = -1.0f;
			depthStencilState.maxDepthBounds = 1.0f;
			depthStencilState.stencilTestEnable = VK_FALSE;
			depthStencilState.front = {};
			depthStencilState.back = {};
		}

		std::vector<VkDynamicState> dynamicStates = {
			VK_DYNAMIC_STATE_VIEWPORT,
			VK_DYNAMIC_STATE_SCISSOR
		};
		VkPipelineDynamicStateCreateInfo dynamicState = {};
		dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
		dynamicState.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
		dynamicState.pDynamicStates = dynamicStates.data();
		
		std::vector<VkPipelineShaderStageCreateInfo> stages = {};
		for (auto& [type, shader] : desc.ShaderProgram->GetShaders())
			stages.emplace_back(static_cast<ShaderVK*>(shader)->GetStage());
		
		VkGraphicsPipelineCreateInfo createInfo = {};
		createInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
		createInfo.stageCount = static_cast<uint32_t>(stages.size());
		createInfo.pStages = stages.data();
		createInfo.pVertexInputState = &vertexInputState;
		createInfo.pInputAssemblyState = &inputAssemblyState;
		createInfo.pViewportState = &viewportState;
		createInfo.pRasterizationState = &rasterizationState;
		createInfo.pMultisampleState = &multisampleState;
		createInfo.pColorBlendState = &colorBlendState;
		createInfo.pDepthStencilState = &depthStencilState;
		createInfo.layout = static_cast<ShaderProgramVK*>(desc.ShaderProgram)->GetPipelineLayout();
		createInfo.renderPass = renderpass;
		createInfo.subpass = 0;
		createInfo.pDynamicState = &dynamicState;
		createInfo.basePipelineHandle = VK_NULL_HANDLE;
		createInfo.basePipelineIndex = -1;

		if (vkCreateGraphicsPipelines(m_Device->Get(), VK_NULL_HANDLE, 1, &createInfo, nullptr, &m_Handle) != VK_SUCCESS)
			throw std::runtime_error("Failed to create graphics pipeline");
	}

	PipelineVK::PipelineVK(GraphicsDeviceVK* device, const ComputePipelineDesc& desc)
		: Pipeline(desc), m_Device(device), m_Handle(VK_NULL_HANDLE)
	{
		throw std::runtime_error("Compute shaders are not supported at the moment!");
	}

	PipelineVK::PipelineVK(GraphicsDeviceVK* device, const RaytracingPipelineDesc& desc)
		: Pipeline(desc), m_Device(device), m_Handle(VK_NULL_HANDLE)
	{
		throw std::runtime_error("Raytracing is not supported at the moment!");
	}

	PipelineVK::~PipelineVK()
	{
		vkDestroyPipeline(m_Device->Get(), m_Handle, nullptr);
	}
}
