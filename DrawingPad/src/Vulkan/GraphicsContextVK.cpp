#include "pwpch.h"]
#include "GraphicsContextVK.h"

#include "RenderPassVK.h"
#include "FramebufferPoolVK.h"
#include "RenderPassPoolVK.h"
#include "PipelineVK.h"
#include "BufferVK.h"

namespace VkAPI {
	GraphicsContextVK::GraphicsContextVK(GraphicsDeviceVK* device, const GraphicsContextDesc& desc)
		: GraphicsContext(device, desc), m_vkDevice(device)
	{
		for (auto i = 0; i < 3; i++)
		{
			m_CmdPool[i] = new CommandPoolVK(device, device->GetGraphicsIndex(), 0);
			m_DescPool[i] = new DescriptorSetPoolVK(device);
		}
	}

	void GraphicsContextVK::Flush()
	{
		std::vector<VkCommandBuffer> buffs;
		buffs.reserve(1);
		auto buff = m_CommandBuffer.Get();
		if (buff != VK_NULL_HANDLE)
		{
			if (m_CommandBuffer.GetState().RenderPass != VK_NULL_HANDLE)
				m_CommandBuffer.EndRenderPass();

			m_CommandBuffer.EndCommandBuffer();

			buffs.emplace_back(buff);
		}

		// Add the imageAvailable Semaphores to the wait Semaphores so that
		// the command buffer does not submit until the image has been acquired.
		// Add the renderFinished Semaphores to the signal Semaphores so that the
		// program can be notified when the rendering is finished

		VkSubmitInfo subInfo = {};
		subInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		subInfo.commandBufferCount = static_cast<uint32_t>(buffs.size());
		subInfo.pCommandBuffers = buffs.data();
		VkPipelineStageFlags mask[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
		subInfo.pWaitDstStageMask = mask;
		subInfo.waitSemaphoreCount = static_cast<uint32_t>(m_WaitSemaphores.size());
		subInfo.pWaitSemaphores = subInfo.waitSemaphoreCount != 0 ? m_WaitSemaphores.data() : nullptr;
		subInfo.signalSemaphoreCount = static_cast<uint32_t>(m_SignalSemaphores.size());
		subInfo.pSignalSemaphores = subInfo.signalSemaphoreCount != 0 ? m_SignalSemaphores.data() : nullptr;

		// Reset the inFlightFence for the current frame.
		vkResetFences(m_vkDevice->Get(), 1, m_SubFence);

		// Add the inFlightFence for current frame to allow for signaling of when
		// the render is finished for the CPU.
		m_vkDevice->SubmitCommandBuffer(subInfo, m_SubFence);

		m_WaitSemaphores.clear();
		m_SignalSemaphores.clear();

		if (buff != VK_NULL_HANDLE)
			DisposeCurrentBuffer();

		m_CommandBuffer.Reset();
		for (uint32_t i = 0; i < m_NumVertexBuffers; i++)
			m_VertexBuffers[i] = nullptr;
		m_NumVertexBuffers = 0;
	}

	void GraphicsContextVK::Begin(uint32_t frameIdx)
	{
		m_FrameIndex = frameIdx;
		m_CmdPool[m_FrameIndex]->Reset();
	}

	void GraphicsContextVK::BeginRenderPass(const BeginRenderPassAttribs& attribs)
	{
		VkRenderPass pass = ((RenderPassVK*)attribs.RenderPass)->GetRenderPass();
		VkFramebuffer fb = ((FramebufferVK*)attribs.Framebuffer)->Get();

		std::vector<VkClearValue> clearValues;
		if (attribs.ClearCount > 0)
		{
			clearValues.resize(attribs.ClearCount);
			for (uint32_t i = 0; i < attribs.ClearCount; i++) {
				clearValues[i].color.float32[0] = attribs.ClearValues[i].r;
				clearValues[i].color.float32[1] = attribs.ClearValues[i].g;
				clearValues[i].color.float32[2] = attribs.ClearValues[i].b;
				clearValues[i].color.float32[3] = attribs.ClearValues[i].a;
			}
		}

		VerifyCommandBuffer();
		m_CommandBuffer.BeginRenderPass(pass, fb, m_FramebufferWidth, m_FramebufferHeight, static_cast<uint32_t>(clearValues.size()), clearValues.data());

		SetViewports(1, nullptr, 0, 0);
	}

	void GraphicsContextVK::EndRenderPass()
	{
		m_CommandBuffer.EndRenderPass();
	}

	void GraphicsContextVK::ClearColor(TextureView* tv, const float* color)
	{
		static constexpr float defaultColor[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
		if (color == nullptr)
			color = defaultColor;

		VerifyCommandBuffer();

		const auto& desc = tv->GetDesc();

		static constexpr const uint32_t InvalidIndex = ~uint32_t{ 0 };
		uint32_t attachIdx = InvalidIndex;
		for (uint32_t i = 0; i < m_NumRenderTargets; i++)
			if (m_RenderTargets[i] == tv)
			{
				attachIdx = i;
				break;
			}

		if (attachIdx != InvalidIndex)
		{
			const auto& state = m_CommandBuffer.GetState();
			if (state.Framebuffer != m_vkFramebuffer)
			{
				VkClearValue clear = {};
				clear.color.float32[0] = color[0];
				clear.color.float32[1] = color[1];
				clear.color.float32[2] = color[2];
				clear.color.float32[3] = color[3];

				if (state.RenderPass != VK_NULL_HANDLE)
					m_CommandBuffer.EndRenderPass();
				if (m_vkFramebuffer != VK_NULL_HANDLE)
					m_CommandBuffer.BeginRenderPass(m_vkRenderPass, m_vkFramebuffer, m_FramebufferWidth, m_FramebufferHeight, 1, &clear);
			}
		}
		else
		{
			if (m_CommandBuffer.GetState().RenderPass != VK_NULL_HANDLE)
				m_CommandBuffer.EndRenderPass();

			VkClearColorValue clear = {};
			clear.float32[0] = color[0];
			clear.float32[1] = color[1];
			clear.float32[2] = color[2];
			clear.float32[3] = color[3];

			VkImageSubresourceRange sub = {};
			sub.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			sub.baseArrayLayer = desc.FirstSlice;
			sub.layerCount = desc.Slices;
			sub.baseMipLevel = desc.HighestMip;
			sub.levelCount = desc.NumMipLevels;

			vkCmdClearColorImage(m_CommandBuffer.Get(), ((TextureViewVK*)tv)->GetTexture()->GetImage(), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, &clear, 1, &sub);
		}
	}

	void GraphicsContextVK::ClearDepth(TextureView* tv, ClearDepthStencil clearFlags, float depth, uint8_t stencil)
	{
		auto* dsv = (TextureViewVK*)tv;
		VerifyCommandBuffer();
		bool attachment = dsv == m_DepthStencil;
		if (attachment)
		{
			const auto& state = m_CommandBuffer.GetState();
			if (state.Framebuffer != m_vkFramebuffer)
			{
				if (state.RenderPass != VK_NULL_HANDLE)
					m_CommandBuffer.EndRenderPass();
				if (m_vkFramebuffer != VK_NULL_HANDLE)
					m_CommandBuffer.BeginRenderPass(m_vkRenderPass, m_vkFramebuffer, m_FramebufferWidth, m_FramebufferHeight);
			}

			VkClearAttachment clearAttach = {};
			clearAttach.aspectMask = 0;
			if ((uint32_t)clearFlags & (uint32_t)ClearDepthStencil::Depth) clearAttach.aspectMask |= VK_IMAGE_ASPECT_DEPTH_BIT;
			if ((uint32_t)clearFlags & (uint32_t)ClearDepthStencil::Stencil) clearAttach.aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
			clearAttach.colorAttachment = VK_ATTACHMENT_UNUSED;
			clearAttach.clearValue.depthStencil.depth = depth;
			clearAttach.clearValue.depthStencil.stencil = stencil;
			VkClearRect clearRect;
			clearRect.rect = { {0,0}, {m_FramebufferWidth, m_FramebufferHeight} };
			clearRect.baseArrayLayer = 0;
			clearRect.layerCount = tv->GetDesc().Slices;
			vkCmdClearAttachments(m_CommandBuffer.Get(), 1, &clearAttach, 1, &clearRect);
		}
		else
		{
			if (m_CommandBuffer.GetState().RenderPass != VK_NULL_HANDLE)
				m_CommandBuffer.EndRenderPass();

			auto* tex = (TextureVK*)dsv->GetTexture();
			VkClearDepthStencilValue clearVal = {};
			clearVal.depth = depth;
			clearVal.stencil = stencil;
			VkImageSubresourceRange subres;
			subres.aspectMask = 0;
			if ((uint32_t)clearFlags & (uint32_t)ClearDepthStencil::Depth) subres.aspectMask |= VK_IMAGE_ASPECT_DEPTH_BIT;
			if ((uint32_t)clearFlags & (uint32_t)ClearDepthStencil::Stencil) subres.aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
			subres.baseArrayLayer = tv->GetDesc().FirstSlice;
			subres.layerCount = tv->GetDesc().Slices;
			subres.baseMipLevel = tv->GetDesc().HighestMip;
			subres.levelCount = tv->GetDesc().NumMipLevels;
			vkCmdClearDepthStencilImage(m_CommandBuffer.Get(), tex->GetImage(), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, &clearVal, 1, &subres);
		}
	}
	
	void GraphicsContextVK::SetViewports(uint32_t num, const Viewport* viewports, uint32_t width, uint32_t height)
	{
		if (width == 0 || height == 0)
		{
			width = m_FramebufferWidth;
			height = m_FramebufferHeight;
		}

		Viewport defaultVP{ 0, 0, static_cast<float>(width), static_cast<float>(height) };
		if (num == 1 && viewports == nullptr)
		{
			viewports = &defaultVP;
		}

		VkViewport vp[MAX_VIEWPORTS];
		for (uint32_t i = 0; i < num; i++) {
			vp[i].x = viewports[i].X;
			vp[i].y = viewports[i].Y;
			vp[i].width = viewports[i].Width;
			vp[i].height = viewports[i].Height;
			vp[i].minDepth = viewports[i].minDepth;
			vp[i].maxDepth = viewports[i].maxDepth;
		}

		VerifyCommandBuffer();
		m_CommandBuffer.SetViewports(0, num, vp);
	}

	void GraphicsContextVK::SetScissors(uint32_t num, const Viewport* viewports, uint32_t width, uint32_t height)
	{
		if (width == 0 || height == 0)
		{
			width = m_FramebufferWidth;
			height = m_FramebufferHeight;
		}

		Viewport defaultVP{ 0, 0, static_cast<float>(width), static_cast<float>(height) };
		if (num == 1 && viewports == nullptr)
		{
			viewports = &defaultVP;
		}

		VkRect2D vp[MAX_VIEWPORTS];
		for (uint32_t i = 0; i < num; i++) {
			vp[i].offset = { 0,0 };
			vp[i].extent = { width, height };
		}

		VerifyCommandBuffer();
		m_CommandBuffer.SetScissors(0, num, vp);
	}
	
	void GraphicsContextVK::SetRenderTargets(uint32_t numTargets, TextureView** targets, TextureView* depthStencil)
	{
		if (numTargets == 0 && depthStencil == nullptr)
		{
			for (uint32_t i = 0; i < m_NumRenderTargets; i++)
				m_RenderTargets[i] = nullptr;
			m_NumRenderTargets = 0;
			m_FramebufferWidth = 0;
			m_FramebufferHeight = 0;
			m_FramebufferSlices = 0;
			m_FramebufferSamples = 0;
			m_DepthStencil = nullptr;

			m_vkRenderPass = VK_NULL_HANDLE;
			m_vkFramebuffer = VK_NULL_HANDLE;
			return;
		}

		if (numTargets != m_NumRenderTargets)
		{
			m_NumRenderTargets = numTargets;
		}

		for (uint32_t i = 0; i < numTargets; i++) {
			TextureView* view = targets[i];
			if (view)
			{
				const auto& desc = view->GetDesc();
				if (m_FramebufferWidth == 0)
				{
					auto* tex = view->GetTexture();
					const auto& texDesc = tex->GetDesc();
					m_FramebufferWidth = std::max(texDesc.Width >> desc.HighestMip, 1U);
					m_FramebufferHeight = std::max(texDesc.Height >> desc.HighestMip, 1U);
					m_FramebufferSlices = desc.Slices;
					m_FramebufferSamples = texDesc.SampleCount;
				}
			}
			if (m_RenderTargets[i] != view)
			{
				m_RenderTargets[i] = view;
			}
		}

		if (m_DepthStencil != depthStencil)
			m_DepthStencil = depthStencil;

		FBKey fKey = {};
		RPKey rKey = {};
		if (m_DepthStencil)
		{
			TextureVK* depth = ((TextureVK*)m_DepthStencil->GetTexture());
			rKey.SampleCount = static_cast<uint8_t>(depth->GetDesc().SampleCount);
			rKey.DepthFormat = depth->GetDesc().Format;
		}

		fKey.AttachmentCount = m_NumRenderTargets + (m_DepthStencil ? 1 : 0);
		rKey.NumColors = static_cast<uint8_t>(m_NumRenderTargets);

		for (uint32_t i = 0; i < m_NumRenderTargets; i++) {
			if (auto* rt = m_RenderTargets[i])
			{
				auto* target = rt->GetTexture();
				fKey.Attachments[i] = ((TextureViewVK*)rt)->GetView();
				rKey.ColorFormats[i] = target->GetDesc().Format;
				if (rKey.SampleCount == 0)
					rKey.SampleCount = static_cast<uint8_t>(target->GetDesc().SampleCount);
			}
			else
			{
				fKey.Attachments[i] = VK_NULL_HANDLE;
				rKey.ColorFormats[i] = TextureFormat::Unknown;
			}
		}

		auto& fbPool = ((GraphicsDeviceVK*)m_Device)->GetFramebufferPool();
		auto& rpPool = ((GraphicsDeviceVK*)m_Device)->GetRenderPassPool();

		m_vkRenderPass = rpPool.GetRenderPass(rKey)->GetRenderPass();
		fKey.Pass = m_vkRenderPass;
		fKey.CommandQueueMask = ~uint64_t{ 0 };
		m_vkFramebuffer = fbPool.GetFramebuffer(fKey, m_FramebufferWidth, m_FramebufferHeight, m_FramebufferSlices);

		SetViewports(1, nullptr, 0, 0);
		SetScissors(1, nullptr, 0, 0);
	}
	
	void GraphicsContextVK::SetPipeline(Pipeline* pipeline)
	{
		PipelineVK* vkPipeline = (PipelineVK*)pipeline;
		vkCmdBindPipeline(m_CommandBuffer.Get(), VK_PIPELINE_BIND_POINT_GRAPHICS, vkPipeline->Get());
	}
	
	void GraphicsContextVK::SetVertexBuffers(uint32_t start, uint32_t num, Buffer** buffers, const uint32_t* offset)
	{
		for (uint32_t i = m_NumVertexBuffers, j = 0; i < m_NumVertexBuffers + num; i++, j++)
			m_VertexBuffers[i] = buffers[j];
		m_NumVertexBuffers += num;
	}
	
	void GraphicsContextVK::SetIndexBuffer(Buffer* buffer, uint32_t offset)
	{
		m_IndexBuffer = buffer;
	}
	
	void GraphicsContextVK::Draw(const DrawAttribs& attribs)
	{
		VerifyCommandBuffer();
		vkCmdDraw(m_CommandBuffer.Get(), attribs.VrtIdxCount, attribs.InstanceCount, attribs.FirstVrtIdx, attribs.FirstInstance);
	}
	
	void GraphicsContextVK::DrawIndexed(const DrawAttribs& attribs)
	{
		std::vector<VkBuffer> buffs;
		buffs.resize(m_NumVertexBuffers);
		for (uint32_t i = 0; i < m_NumVertexBuffers; i++)
			buffs[i] = ((BufferVK*)m_VertexBuffers[i])->Get();
		VkDeviceSize offsets[] = { 0 };
		vkCmdBindVertexBuffers(m_CommandBuffer.Get(), 0, m_NumVertexBuffers, buffs.data(), offsets);
		vkCmdBindIndexBuffer(m_CommandBuffer.Get(), ((BufferVK*)m_IndexBuffer)->Get(), attribs.VertexOffset, VK_INDEX_TYPE_UINT16);
		vkCmdDrawIndexed(m_CommandBuffer.Get(), attribs.VrtIdxCount, attribs.InstanceCount, attribs.FirstVrtIdx, attribs.VertexOffset, attribs.FirstVrtIdx);
	}
	
	void GraphicsContextVK::DrawIndirect(const DrawIndirectAttribs& attribs)
	{
	}
	
	void GraphicsContextVK::DrawIndexedIndirect(const DrawIndirectAttribs& attribs)
	{
	}
	
	void GraphicsContextVK::Dispatch(const DispatchAttribs& attribs)
	{
	}
	
	void GraphicsContextVK::DispatchIndirect(const DispatchIndirectAttribs& attribs)
	{
	}
	
	void GraphicsContextVK::ResourceBarrier()
	{
	}
	
	void GraphicsContextVK::UploadBuffer(Buffer* src, uint32_t offset, size_t size, void* data)
	{
		void* temp;
		vkMapMemory(m_vkDevice->Get(), ((BufferVK*)src)->GetMemory(), offset, size, 0, &temp);
		memcpy(temp, data, size);
		vkUnmapMemory(m_vkDevice->Get(), ((BufferVK*)src)->GetMemory());
	}

	void GraphicsContextVK::CopyBuffer(Buffer* src, Buffer* dst, uint32_t size)
	{
		VerifyCommandBuffer();
		VkBufferCopy copy = {};
		copy.size = size;
		vkCmdCopyBuffer(m_CommandBuffer.Get(), ((BufferVK*)src)->Get(), ((BufferVK*)dst)->Get(), 1, &copy);

		Flush();
		vkQueueWaitIdle(((GraphicsDeviceVK*)m_Device)->GetGraphicsQueue());
	}
	
	void GraphicsContextVK::UploadTexture()
	{
	}

	void GraphicsContextVK::VerifyCommandBuffer()
	{
		if (m_CommandBuffer.Get() == VK_NULL_HANDLE)
			m_CommandBuffer.SetBuffer(m_CmdPool[m_FrameIndex]->GetBuffer());
	}

	void GraphicsContextVK::TransitionTexture(TextureVK* tex, ViewType oldState, ViewType newState)
	{
		//if (oldState == ViewType::Undefined)
		//	oldState = tex.GetState();
		//
		//VerifyCommandBuffer();
		//
		//auto img = tex->GetImage();
		//VkImageSubresourceRange subRange;
		//subRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		//subRange.baseArrayLayer = 0;
		//subRange.layerCount = VK_REMAINING_ARRAY_LAYERS;
		//subRange.baseMipLevel = 0;
		//subRange.levelCount = VK_REMAINING_MIP_LEVELS;
		//
		//auto oldLayout = UtilsVK::StateToImageLayout(oldState);
		//auto newLayout = UtilsVK::StateToImageLayout(newState);
		//auto oldStages = UtilsVK::StateToPipelineStage(oldState);
		//auto newStages = UtilsVK::StateToPipelineStage(newState);
		//
		//VkImageMemoryBarrier imgBarrier;
		//imgBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		//imgBarrier.oldLayout = oldLayout;
		//imgBarrier.newLayout = newLayout;
		//imgBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		//imgBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		//imgBarrier.image = img;
		//imgBarrier.subresourceRange = subRange;
		//imgBarrier.srcAccessMask = UtilsVK::ImageLayoutToAccessMask(oldLayout);
		//imgBarrier.dstAccessMask = UtilsVK::ImageLayoutToAccessMask(newLayout);
		//
		//VkPipelineStageFlags srcStages;
		//VkPipelineStageFlags dstStages;
		//
		//if (newLayout == VK_IMAGE_LAYOUT_PRESENT_SRC_KHR)
		//{
		//	srcStages = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
		//	dstStages = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
		//}
		//else if (imgBarrier.dstAccessMask != 0)
		//{
		//	srcStages = UtilsVK::AccessFlagsToPipelineStage(imgBarrier.srcAccessMask);
		//	dstStages = UtilsVK::AccessFlagsToPipelineStage(imgBarrier.dstAccessMask);
		//}
		//else
		//{
		//	srcStages = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
		//	dstStages = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
		//}
		//
		//vkCmdPipelineBarrier(m_CommandBuffer, srcStages, dstStages, 0, 0, nullptr, 0, nullptr, 1, &imgBarrier);
	}

	void GraphicsContextVK::DisposeCurrentBuffer()
	{
		if (m_CommandBuffer.GetState().RenderPass != VK_NULL_HANDLE)
			throw new std::runtime_error("Disposing command buffer while renderpass active");
		auto buf = m_CommandBuffer.Get();
		if (buf != VK_NULL_HANDLE)
		{
			if (m_CmdPool[m_FrameIndex] != nullptr)
				m_CmdPool[m_FrameIndex]->ReturnBuffer(std::move(buf));
			m_CommandBuffer.Reset();
		}
	}
}