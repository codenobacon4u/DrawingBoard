#include "pwpch.h"
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
			m_CmdPool[i] = DBG_NEW CommandPoolVK(device, device->GetGraphicsIndex(), 0);
			m_DescPool[i] = DBG_NEW DescriptorSetPoolVK(device);
		}
	}

	GraphicsContextVK::~GraphicsContextVK()
	{
		std::cout << "DELETING GRAPHICS CONTEXT" << std::endl;
		vkDeviceWaitIdle(m_vkDevice->Get());
		for (auto i = 0; i < 3; i++)
		{
			delete m_CmdPool[i];
			delete m_DescPool[i];
		}
		for (auto view : m_RenderTargets)
			delete view;
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
		m_DescPool[m_FrameIndex]->ResetPools();
		SetRenderTargets(0, nullptr, nullptr);
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
		static constexpr float defaultColor[4] = { 0.3f, 0.0f, 0.3f, 0.1f };
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
				VkClearValue clear[2] = {};
				clear[0].depthStencil = { 1.0f, 0 };
				clear[1].color.float32[0] = 0.f;//color[0];
				clear[1].color.float32[1] = 0.f;//color[1];
				clear[1].color.float32[2] = 0.f;//color[2];
				clear[1].color.float32[3] = 1.f;//color[3];

				if (state.RenderPass != VK_NULL_HANDLE)
					m_CommandBuffer.EndRenderPass();
				if (m_vkFramebuffer != VK_NULL_HANDLE)
					m_CommandBuffer.BeginRenderPass(m_vkRenderPass, m_vkFramebuffer, m_FramebufferWidth, m_FramebufferHeight, 2, clear);
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
		uint32_t base = 0;
		if (m_DepthStencil)
		{
			TextureVK* depth = ((TextureVK*)m_DepthStencil->GetTexture());
			rKey.SampleCount = static_cast<uint8_t>(depth->GetDesc().SampleCount);
			rKey.DepthFormat = depth->GetDesc().Format;
			fKey.Attachments[base] = static_cast<TextureViewVK*>(m_DepthStencil)->GetView();
			base++;
		}

		fKey.AttachmentCount = m_NumRenderTargets + (m_DepthStencil ? 1 : 0);
		rKey.NumColors = static_cast<uint8_t>(m_NumRenderTargets);

		for (uint32_t i = base; i < m_NumRenderTargets + base; i++) {
			if (auto* rt = m_RenderTargets[i-base])
			{
				auto* target = rt->GetTexture();
				fKey.Attachments[i] = ((TextureViewVK*)rt)->GetView();
				rKey.ColorFormats[i-base] = target->GetDesc().Format;
				if (rKey.SampleCount == 0)
					rKey.SampleCount = static_cast<uint8_t>(target->GetDesc().SampleCount);
			}
			else
			{
				fKey.Attachments[i] = VK_NULL_HANDLE;
				rKey.ColorFormats[i-base] = TextureFormat::Unknown;
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
		m_Pipeline = pipeline;
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

	void GraphicsContextVK::SetShaderResource(ResourceBindingType type, uint32_t set, uint32_t binding, Buffer* buffer)
	{
		switch (type)
		{
		case ResourceBindingType::UniformBuffer:
			m_Bindings.SetLayouts[set].UniformBuffers |= 1 << binding;
			m_Bindings.SetCount = std::max(m_Bindings.SetCount, set+1);
			break;
		case ResourceBindingType::StorageBuffer:
			m_Bindings.SetLayouts[set].StorageBuffers |= 1 << binding;
			m_Bindings.SetCount = std::max(m_Bindings.SetCount, set+1);
			break;
		case ResourceBindingType::SampledBuffer:
			m_Bindings.SetLayouts[set].SampledBuffers |= 1 << binding;
			m_Bindings.SetCount = std::max(m_Bindings.SetCount, set+1);
			break;
		default:
			return;
		}
		BufferVK* buff = (BufferVK*)buffer;
		auto& resource = m_Bindings.Sets[set][binding].Buffer;
		resource.buffer = buff->Get();
		resource.offset = m_FrameIndex * (buffer->GetSize() / 3);
		resource.range = buff->GetSize() / 3;
		m_UpdateSetsMask |= 1 << set;
	}

	void GraphicsContextVK::SetShaderResource(ResourceBindingType type, uint32_t set, uint32_t binding, Texture* texture)
	{
		switch (type)
		{
		case ResourceBindingType::Image:
			m_Bindings.SetLayouts[set].SeparateImages |= 1 << binding;
			m_Bindings.SetCount = std::max(m_Bindings.SetCount, set+1);
			break;
		case ResourceBindingType::ImageSampler:
			m_Bindings.SetLayouts[set].SampledImages |= 1 << binding;
			m_Bindings.SetCount = std::max(m_Bindings.SetCount, set+1);
			break;
		case ResourceBindingType::ImageStorage:
			m_Bindings.SetLayouts[set].StorageImages |= 1 << binding;
			m_Bindings.SetCount = std::max(m_Bindings.SetCount, set+1);
			break;
		case ResourceBindingType::Sampler:
			m_Bindings.SetLayouts[set].Samplers |= 1 << binding;
			m_Bindings.SetCount = std::max(m_Bindings.SetCount, set+1);
			break;
		default:
			return;
		}
		TextureVK* tex = (TextureVK*)texture;
		auto& resource = m_Bindings.Sets[set][binding].Texture;
		resource.imageView = ((TextureViewVK*)tex->GetDefaultView())->GetView();
		resource.sampler = tex->GetSampler();
		resource.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		m_UpdateSetsMask |= 1 << set;
	}
	
	void GraphicsContextVK::Draw(const DrawAttribs& attribs)
	{
		PrepareDraw();
		vkCmdDraw(m_CommandBuffer.Get(), attribs.VrtIdxCount, attribs.InstanceCount, attribs.FirstVrtIdx, attribs.FirstInstance);
	}
	
	void GraphicsContextVK::DrawIndexed(const DrawAttribs& attribs)
	{
		PrepareDraw();
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

	void GraphicsContextVK::PrepareDraw()
	{
		VerifyCommandBuffer();
		std::vector<VkBuffer> buffs;
		buffs.resize(m_NumVertexBuffers);
		for (uint32_t i = 0; i < m_NumVertexBuffers; i++)
			buffs[i] = ((BufferVK*)m_VertexBuffers[i])->Get();
		VkDeviceSize offsets[] = { 0 };
		vkCmdBindVertexBuffers(m_CommandBuffer.Get(), 0, m_NumVertexBuffers, buffs.data(), offsets);
		
		BindDescriptorSets();
	}

	void GraphicsContextVK::BindDescriptorSets()
	{
		PipelineVK* pipe = (PipelineVK*)m_Pipeline;
		for (uint32_t set = 0; set < m_Bindings.SetCount; set++)
		{
			uint32_t* stages = pipe->GetProgram()->GetLayout().BindingStagesMask[set];
			auto binds = m_Bindings.SetLayouts[set];
			VkDescriptorSetLayout layout = pipe->GetProgram()->GetCache()->GetLayout(binds, stages);
			auto result = m_DescPool[m_FrameIndex]->RequestDescriptorSet(layout, binds, stages);

			//if (!result.first)
			UpdateDescriptorSet(result.second, binds, m_Bindings.Sets[set]);

			vkCmdBindDescriptorSets(m_CommandBuffer.Get(), VK_PIPELINE_BIND_POINT_GRAPHICS, pipe->GetProgram()->GetPipelineLayout(), set, 1, &result.second, 0, nullptr);
		}
	}

	void GraphicsContextVK::UpdateDescriptorSet(VkDescriptorSet set, const DSLKey& key, ShaderResourceVK* bindings)
	{
		std::vector<VkWriteDescriptorSet> writes;

		for (uint32_t binding = 0; binding < 32; binding++)
		{
			if (key.UniformBuffers & 1 << binding)
			{
				VkWriteDescriptorSet write = {};
				write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
				write.dstSet = set;
				write.dstBinding = binding;
				write.dstArrayElement = 0;
				write.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
				write.descriptorCount = 1;
				write.pBufferInfo = &bindings[binding].Buffer;
				writes.push_back(std::move(write));
			}
			if (key.StorageBuffers & 1 << binding)
			{
				VkWriteDescriptorSet write = {};
				write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
				write.dstSet = set;
				write.dstBinding = binding;
				write.dstArrayElement = 0;
				write.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
				write.descriptorCount = 1;
				write.pBufferInfo = &bindings[binding].Buffer;
				writes.push_back(std::move(write));
			}
			if (key.SampledBuffers & 1 << binding)
			{
				VkWriteDescriptorSet write = {};
				write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
				write.dstSet = set;
				write.dstBinding = binding;
				write.dstArrayElement = 0;
				write.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER;
				write.descriptorCount = 1;
				write.pBufferInfo = &bindings[binding].Buffer;
				writes.push_back(std::move(write));
			}
			if (key.SampledImages & 1 << binding)
			{
				VkWriteDescriptorSet write = {};
				write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
				write.dstSet = set;
				write.dstBinding = binding;
				write.dstArrayElement = 0;
				write.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
				write.descriptorCount = 1;
				write.pImageInfo = &bindings[binding].Texture;
				writes.push_back(std::move(write));
			}
			if (key.SeparateImages & 1 << binding)
			{
				VkWriteDescriptorSet write = {};
				write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
				write.dstSet = set;
				write.dstBinding = binding;
				write.dstArrayElement = 0;
				write.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
				write.descriptorCount = 1;
				write.pImageInfo = &bindings[binding].Texture;
				writes.push_back(std::move(write));
			}
			if (key.Samplers & 1 << binding)
			{
				VkWriteDescriptorSet write = {};
				write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
				write.dstSet = set;
				write.dstBinding = binding;
				write.dstArrayElement = 0;
				write.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER;
				write.descriptorCount = 1;
				write.pImageInfo = &bindings[binding].Texture;
				writes.push_back(std::move(write));
			}
			if (key.StorageImages & 1 << binding)
			{
				VkWriteDescriptorSet write = {};
				write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
				write.dstSet = set;
				write.dstBinding = binding;
				write.dstArrayElement = 0;
				write.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
				write.descriptorCount = 1;
				write.pImageInfo = &bindings[binding].Texture;
				writes.push_back(std::move(write));
			}
			if (key.InputAttachments & 1 << binding)
			{
				VkWriteDescriptorSet write = {};
				write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
				write.dstSet = set;
				write.dstBinding = binding;
				write.dstArrayElement = 0;
				write.descriptorType = VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT;
				write.descriptorCount = 1;
				write.pImageInfo = &bindings[binding].Texture;
				writes.push_back(std::move(write));
			}
		}
		vkUpdateDescriptorSets(((GraphicsDeviceVK*)m_Device)->Get(), static_cast<uint32_t>(writes.size()), writes.data(), 0, nullptr);
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
			throw DBG_NEW std::runtime_error("Disposing command buffer while renderpass active");
		auto buf = m_CommandBuffer.Get();
		if (buf != VK_NULL_HANDLE)
		{
			if (m_CmdPool[m_FrameIndex] != nullptr)
				m_CmdPool[m_FrameIndex]->ReturnBuffer(std::move(buf));
			m_CommandBuffer.Reset();
		}
	}
}