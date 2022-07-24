#include "dppch.h"
#include "GraphicsContextVK.h"

#include "GraphicsDeviceVK.h"

namespace Vulkan 
{
	GraphicsContextVK::GraphicsContextVK(GraphicsDevice* device, Swapchain* swap)
		: GraphicsContext(device, swap), 
		m_Device(static_cast<GraphicsDeviceVK*>(device)), 
		m_Swap(static_cast<SwapchainVK*>(swap)), 
		m_Queue(m_Device->GetGraphicsQueue())
	{
		VkSemaphoreCreateInfo semInfo = {};
		semInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

		VkFenceCreateInfo fenceInfo = {};
		fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
		fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

		m_Frames.resize(m_Swap->GetBackbufferCount());
		for (uint32_t i = 0; i < m_Frames.size(); i++) {
			m_Frames[i].CommandPools.resize(1);
			m_Frames[i].Index = i;
			m_Frames[i].CommandPools[0][m_Device->GetGraphicsIndex()] = (DBG_NEW CommandPoolVK(m_Device, m_Device->GetGraphicsIndex(), 0));
			m_Frames[i].DescriptorPool = DBG_NEW DescriptorSetPoolVK(m_Device);
			vkCreateSemaphore(m_Device->Get(), &semInfo, nullptr, &m_Frames[i].ImageAcquired);
			vkCreateSemaphore(m_Device->Get(), &semInfo, nullptr, &m_Frames[i].RenderFinished);
			vkCreateFence(m_Device->Get(), &fenceInfo, nullptr, &m_Frames[i].FrameFence);
		}
	}

	GraphicsContextVK::~GraphicsContextVK()
	{
		for (auto& frame : m_Frames) {
			delete frame.DescriptorPool;
			vkDestroySemaphore(m_Device->Get(), frame.ImageAcquired, nullptr);
			vkDestroySemaphore(m_Device->Get(), frame.RenderFinished, nullptr);
			vkDestroyFence(m_Device->Get(), frame.FrameFence, nullptr);
			for (auto& pools : frame.CommandPools)
				for (auto& [i, pool] : pools)
					delete pool;
		}
	}

	CommandBuffer* GraphicsContextVK::Begin()
	{
		if (!m_FrameActive) {
			vkWaitForFences(m_Device->Get(), 1, &m_Frames[m_Index].FrameFence, VK_TRUE, UINT64_MAX);

			m_Index = m_Swap->GetImageIndex();
			m_Swap->AcquireNextImage(m_Frames[m_Index].ImageAcquired);
			m_FrameActive = true;

			vkResetFences(m_Device->Get(), 1, &m_Frames[m_Index].FrameFence);
			for (auto& pools : m_Frames[m_Index].CommandPools)
				for (auto& [i, pool] : pools)
					pool->Reset();
		}


		auto pool = GetCommandPool(m_Device->GetGraphicsIndex(), 0);
		return pool->RequestCommandBuffer();
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

		vkQueueSubmit(m_Queue, 1, &subInfo, m_Frames[m_Index].FrameFence);
	}

	void GraphicsContextVK::Present()
	{
		m_Swap->Present(m_Queue, m_Frames[m_Index].RenderFinished);
		m_FrameActive = false;
	}

	CommandBuffer* GraphicsContextVK::GetCommandBuffer(uint32_t queueFamilyIndex, CommandBufferType type, uint32_t threadIndex)
	{
		auto* pool = GetCommandPool(queueFamilyIndex, threadIndex);

		return pool->RequestCommandBuffer(type);
	}

	CommandPool* GraphicsContextVK::GetCommandPool(uint32_t queueFamilyIndex, uint32_t threadIndex)
	{
		auto& pools = m_Frames[m_Index].CommandPools;
		if (pools.size() < threadIndex)
			pools.resize(threadIndex);

		auto& it = pools[threadIndex].find(queueFamilyIndex);
		if (it != pools[threadIndex].end())
			return it->second;

		pools[threadIndex][queueFamilyIndex] = DBG_NEW CommandPoolVK(m_Device, queueFamilyIndex, 0);

		return pools[threadIndex][queueFamilyIndex];
	}
}
