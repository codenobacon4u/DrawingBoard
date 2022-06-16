#include "pwpch.h"
#include "GraphicsContextVK.h"

#include "GraphicsDeviceVK.h"

namespace Vulkan {
	GraphicsContextVK::GraphicsContextVK(GraphicsDevice* device, Swapchain* swap)
		: GraphicsContext(device, swap), m_Device(static_cast<GraphicsDeviceVK*>(device)), m_Swap(static_cast<SwapchainVK*>(swap))
	{
		VkSemaphoreCreateInfo semInfo = {};
		semInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

		VkFenceCreateInfo fenceInfo = {};
		fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
		fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

		m_Frames.resize(m_Swap->GetBackbufferCount());
		for (uint32_t i = 0; i < m_Frames.size(); i++) {
			m_Frames[i].Index = i;
			m_Frames[i].CommandPools.push_back(DBG_NEW CommandPoolVK(m_Device, m_Device->GetGraphicsIndex(), 0));
			m_Frames[i].DescriptorPool = DBG_NEW DescriptorSetPoolVK(m_Device);
			vkCreateSemaphore(m_Device->Get(), &semInfo, nullptr, &m_Frames[i].ImageAcquired);
			vkCreateSemaphore(m_Device->Get(), &semInfo, nullptr, &m_Frames[i].RenderFinished);
			vkCreateFence(m_Device->Get(), &fenceInfo, nullptr, &m_Frames[i].FrameFence);
		}
	}

	GraphicsContextVK::~GraphicsContextVK()
	{
		m_Frames.clear();
	}

	CommandBuffer* GraphicsContextVK::Begin()
	{
		if (!m_FrameActive) {
			vkWaitForFences(m_Device->Get(), 1, &m_Frames[m_Index].FrameFence, VK_TRUE, UINT64_MAX);

			m_Index = m_Swap->GetImageIndex();
			m_Swap->AcquireNextImage(m_Frames[m_Index].ImageAcquired);
			m_FrameActive = true;

			vkResetFences(m_Device->Get(), 1, &m_Frames[m_Index].FrameFence);
			for (auto pool : m_Frames[m_Index].CommandPools)
				pool->Reset();
			m_Frames[m_Index].DescriptorPool->ResetPools();
		}

		return m_Frames[m_Index].CommandPools[0]->RequestCommandBuffer();
	}

	void GraphicsContextVK::Submit(CommandBuffer& commandBuffer)
	{
		Submit({ &commandBuffer });
	}

	void GraphicsContextVK::Submit(const std::vector<CommandBuffer*>& commandBuffers)
	{
		std::vector<VkCommandBuffer> buffs(commandBuffers.size(), VK_NULL_HANDLE);
		std::transform(commandBuffers.begin(), commandBuffers.end(), buffs.begin(), [](CommandBuffer* buff) { return static_cast<CommandBufferVK*>(buff)->Get(); });

		VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };

		VkSubmitInfo subInfo = {};
		subInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		subInfo.commandBufferCount = static_cast<uint32_t>(buffs.size());
		subInfo.pCommandBuffers = buffs.data();
		subInfo.waitSemaphoreCount = 1;
		subInfo.pWaitSemaphores = &m_Frames[m_Index].ImageAcquired;
		subInfo.pWaitDstStageMask = waitStages;
		subInfo.signalSemaphoreCount = 1;
		subInfo.pSignalSemaphores = &m_Frames[m_Index].RenderFinished;

		vkQueueSubmit(m_Device->GetGraphicsQueue(), 1, &subInfo, m_Frames[m_Index].FrameFence);

		End();
	}

	void GraphicsContextVK::End()
	{
		m_Swap->Present(m_Frames[m_Index].RenderFinished);
		m_FrameActive = false;
	}

	CommandBuffer* GraphicsContextVK::GetCommandBuffer(CommandBufferType type, uint32_t threadIndex)
	{
		auto* pools = &m_Frames[m_Index].CommandPools;
		if (pools->size() < threadIndex)
			pools->resize(threadIndex, DBG_NEW CommandPoolVK(m_Device, m_Device->GetGraphicsIndex(), 0));

		return pools->at(threadIndex)->RequestCommandBuffer(type);
	}
}
