#include "pwpch.h"
#include "CommandBufferVK.h"

#include "GraphicsDeviceVK.h"
#include "Vulkan/BufferVK.h"
#include "Vulkan/FramebufferVK.h"
#include "Vulkan/PipelineVK.h"
#include "Vulkan/RenderPassVK.h"

namespace Vulkan {
	CommandBufferVK::CommandBufferVK(GraphicsDeviceVK* device, VkCommandPool pool, VkCommandBufferLevel level)
		: m_Device(device), m_Pool(pool)
	{
		VkCommandBufferAllocateInfo allocInfo = {};
		allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		allocInfo.commandPool = m_Pool;
		allocInfo.level = level;
		allocInfo.commandBufferCount = 1;
		vkAllocateCommandBuffers(m_Device->Get(), &allocInfo, &m_Curr);

		m_DescriptorPool = DBG_NEW DescriptorSetPoolVK(device);
	}

	CommandBufferVK::~CommandBufferVK()
	{
		vkFreeCommandBuffers(m_Device->Get(), m_Pool, 1, &m_Curr);

		delete m_DescriptorPool;
	}

	void CommandBufferVK::Begin()
	{
		VkCommandBufferBeginInfo beginInfo = {};
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		beginInfo.pNext = nullptr;
		beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
		vkBeginCommandBuffer(m_Curr, &beginInfo);
	}

	void CommandBufferVK::BeginRenderPass(RenderPass* renderpass, std::vector<TextureView*> renderTargets, std::vector<ClearValue> clearValues)
	{
		assert(clearValues.size() == renderpass->getDesc().Attachments.size());
		assert(renderTargets.size() > 0);

		uint32_t width = renderTargets[0]->GetTexture()->GetDesc().Width;
		uint32_t height = renderTargets[0]->GetTexture()->GetDesc().Height;

		std::vector<VkImageView> views(renderTargets.size(), VK_NULL_HANDLE);
		std::transform(renderTargets.begin(), renderTargets.end(), views.begin(), [](TextureView* view) { return static_cast<TextureViewVK*>(view)->GetView(); });

		FBKey key = {};
		key.Pass = static_cast<RenderPassVK*>(renderpass)->Get();
		key.AttachmentCount = static_cast<uint32_t>(views.size());
		key.Attachments = views;

		auto framebuffer = m_Device->GetFramebufferPool().GetFramebuffer(key, width, height, 1);

		VkRenderPassBeginInfo beginInfo = {};
		beginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		beginInfo.renderPass = static_cast<RenderPassVK*>(renderpass)->Get();
		beginInfo.framebuffer = framebuffer;
		beginInfo.renderArea = { { 0, 0 }, { width, height } };
		beginInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
		std::vector<VkClearValue> values(clearValues.size());
		for (uint32_t i = 0; i < clearValues.size(); i++)
		{
			if (renderpass->getDesc().Attachments[i].Format == TextureFormat::D32Float ||
				renderpass->getDesc().Attachments[i].Format == TextureFormat::D32FloatS8Uint ||
				renderpass->getDesc().Attachments[i].Format == TextureFormat::D24UnormS8Uint)
			{
				values[i].depthStencil = { clearValues[i].depthStencil.depth, clearValues[i].depthStencil.stencil };
			}
			else {
				values[i].color.float32[0] = clearValues[i].color[0];
				values[i].color.float32[1] = clearValues[i].color[1];
				values[i].color.float32[2] = clearValues[i].color[2];
				values[i].color.float32[3] = clearValues[i].color[3];
			}
		}
		beginInfo.pClearValues = values.data();
		vkCmdBeginRenderPass(m_Curr, &beginInfo, VK_SUBPASS_CONTENTS_INLINE);
	}

	void CommandBufferVK::BindBuffer(Buffer* buffer, uint32_t set, uint32_t binding, uint32_t arrayIndex)
	{
	}

	void CommandBufferVK::BindImage(Texture* texture, uint32_t set, uint32_t binding, uint32_t arrayIndex)
	{
	}

	void CommandBufferVK::BindIndexBuffer(Buffer* buffer, uint64_t offset)
	{
		vkCmdBindIndexBuffer(m_Curr, static_cast<BufferVK*>(buffer)->Get(), static_cast<VkDeviceSize>(offset), VK_INDEX_TYPE_UINT16);
	}

	void CommandBufferVK::BindPipeline(Pipeline* pipeline)
	{
		vkCmdBindPipeline(m_Curr, static_cast<VkPipelineBindPoint>(pipeline->GetBindPoint()), static_cast<PipelineVK*>(pipeline)->Get());
	}

	void CommandBufferVK::BindVertexBuffer(uint32_t start, uint32_t num, std::vector<Buffer*> buffers, std::vector<uint64_t> offsets)
	{
		std::vector<VkBuffer> bufs = {};
		for (auto buffer : buffers)
			bufs.emplace_back(static_cast<BufferVK*>(buffer)->Get());
		vkCmdBindVertexBuffers(m_Curr, start, num, bufs.data(), offsets.data());
	}

	void CommandBufferVK::Draw(uint32_t vertexCount, uint32_t instanceCount, uint32_t firstVertex, uint32_t firstInstance)
	{
		// flush pipeline, push constants, descriptor sets
		vkCmdDraw(m_Curr, vertexCount, instanceCount, firstVertex, firstInstance);
	}

	void CommandBufferVK::DrawIndexed(uint32_t indexCount, uint32_t instanceCount, uint32_t firstIndex, uint32_t vertexOffset, uint32_t firstInstance)
	{
		vkCmdDrawIndexed(m_Curr, indexCount, instanceCount, firstIndex, vertexOffset, firstInstance);
	}

	void CommandBufferVK::DrawIndexedIndirect(Buffer* buffer, uint64_t offset, uint32_t drawCount, uint32_t stride)
	{
		vkCmdDrawIndexedIndirect(m_Curr, static_cast<BufferVK*>(buffer)->Get(), offset, drawCount, stride);
	}

	void CommandBufferVK::DrawIndirect(Buffer* buffer, uint64_t offset, uint32_t drawCount, uint32_t stride)
	{
		vkCmdDrawIndirect(m_Curr, static_cast<BufferVK*>(buffer)->Get(), offset, drawCount, stride);
	}

	void CommandBufferVK::End()
	{
		vkEndCommandBuffer(m_Curr);
	}

	void CommandBufferVK::EndRenderPass()
	{
		vkCmdEndRenderPass(m_Curr);
	}

	void CommandBufferVK::SetScissors(uint32_t first, uint32_t count, std::vector<Rect2D> scissors)
	{
		std::vector<VkRect2D> recs = {};
		for (auto& s : scissors)
			recs.push_back({ {s.offset.x, s.offset.x}, { s.extent.x, s.extent.y } });
		vkCmdSetScissor(m_Curr, first, count, recs.data());
	}

	void CommandBufferVK::SetViewports(uint32_t first, uint32_t count, std::vector<Viewport> viewports)
	{
		std::vector<VkViewport> vkViewports = {};
		for (auto& vp : viewports)
			vkViewports.push_back({ vp.x, vp.y, vp.width, vp.height, vp.minDepth, vp.maxDepth });
		vkCmdSetViewport(m_Curr, first, count, vkViewports.data());
	}
}
