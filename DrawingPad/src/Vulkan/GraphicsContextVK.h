#pragma once
#include "GraphicsContext.h"

#include "CommandPoolVK.h"
#include "DescriptorSetVK.h"
#include "SwapchainVK.h"

namespace Vulkan
{
	typedef struct FrameData {
		uint32_t Index = 0;
		std::vector<CommandPoolVK*> CommandPools = {};
		DescriptorSetPoolVK* DescriptorPool = nullptr;
		VkSemaphore ImageAcquired = VK_NULL_HANDLE;
		VkSemaphore RenderFinished = VK_NULL_HANDLE;
		VkFence FrameFence = VK_NULL_HANDLE;
	} FrameData;

	class GraphicsDeviceVK;
	class GraphicsContextVK : public GraphicsContext
	{
	public:
		GraphicsContextVK(GraphicsDevice* device, Swapchain* swap);
		
		~GraphicsContextVK();

		virtual CommandBuffer* Begin() override;
		virtual void Submit(CommandBuffer& commandBuffer) override;
		virtual void Submit(const std::vector<CommandBuffer*>& commandBuffers) override;
		virtual void Present() override;

		virtual CommandBuffer* GetCommandBuffer(CommandBufferType type = CommandBufferType::Primary, uint32_t threadIndex = 0) override;

	private:
		GraphicsDeviceVK* m_Device;
		SwapchainVK* m_Swap;
		std::vector<FrameData> m_Frames;
		bool m_FrameActive = false;
		uint32_t m_Index = 0;

	};
}
