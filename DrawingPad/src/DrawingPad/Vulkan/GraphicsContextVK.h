#pragma once
#include "DrawingPad/GraphicsContext.h"

#include "CommandPoolVK.h"
#include "DescriptorSetVK.h"
#include "StructsVK.h"
#include "SwapchainVK.h"

namespace DrawingPad
{
	namespace Vulkan
	{
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

			virtual CommandBuffer* GetCommandBuffer(uint32_t queueFamilyIndex, CommandBufferType type = CommandBufferType::Primary, uint32_t threadIndex = 0) override;
		private:
			CommandPool* GetCommandPool(uint32_t queueFamilyIndex, uint32_t threadIndex);
		private:
			GraphicsDeviceVK* m_Device;
			SwapchainVK* m_Swap;

			bool m_FrameActive = false;
			uint32_t m_Index = 0;
			std::vector<FrameData> m_Frames;

			VkQueue m_Queue;
		};
	}
}
