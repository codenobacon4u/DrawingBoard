#include "dppch.h"
#include "CommandBufferVK.h"

#include "GraphicsDeviceVK.h"
#include "Vulkan/BufferVK.h"
#include "Vulkan/PipelineVK.h"
#include "Vulkan/RenderPassVK.h"

namespace Vulkan 
{
	CommandBufferVK::CommandBufferVK(GraphicsDeviceVK* device, VkCommandPool pool, VkCommandBufferLevel level)
		: m_Device(device), m_Pool(pool)
	{
		VkCommandBufferAllocateInfo allocInfo = {};
		allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		allocInfo.commandPool = m_Pool;
		allocInfo.level = level;
		allocInfo.commandBufferCount = 1;
		vkAllocateCommandBuffers(m_Device->Get(), &allocInfo, &m_Handle);

		m_DescriptorPool = DBG_NEW DescriptorSetPoolVK(device);
	}

	CommandBufferVK::~CommandBufferVK()
	{
		vkFreeCommandBuffers(m_Device->Get(), m_Pool, 1, &m_Handle);

		delete m_DescriptorPool;
	}

	void CommandBufferVK::Begin()
	{
		//m_DescriptorPool->ResetPools();
		VkCommandBufferBeginInfo beginInfo = {};
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		beginInfo.pNext = nullptr;
		beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
		vkBeginCommandBuffer(m_Handle, &beginInfo);
	}

	void CommandBufferVK::BeginRenderPass(RenderPass* renderpass, std::vector<TextureView*> renderTargets, std::vector<ClearValue> clearValues)
	{
		//assert(clearValues.size() == renderpass->getDesc().Attachments.size());
		//assert(renderTargets.size() > 0);

		uint32_t width = renderTargets[0]->GetTexture()->GetDesc().Width;
		uint32_t height = renderTargets[0]->GetTexture()->GetDesc().Height;

		std::vector<VkImageView> views(renderTargets.size(), VK_NULL_HANDLE);
		std::transform(renderTargets.begin(), renderTargets.end(), views.begin(), [](TextureView* view) { return static_cast<TextureViewVK*>(view)->Get(); });

		FBKey key = {};
		key.Pass = static_cast<RenderPassVK*>(renderpass)->Get();
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
			if (renderpass->GetDesc().Attachments[i].Format == TextureFormat::D32Float ||
				renderpass->GetDesc().Attachments[i].Format == TextureFormat::D32FloatS8Uint ||
				renderpass->GetDesc().Attachments[i].Format == TextureFormat::D24UnormS8Uint)
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
		vkCmdBeginRenderPass(m_Handle, &beginInfo, VK_SUBPASS_CONTENTS_INLINE);
	}

	void CommandBufferVK::BindBuffer(Buffer* buffer, uint64_t offset, uint64_t range, uint32_t set, uint32_t binding, uint32_t arrayIndex)
	{
		m_DirtySets[set] = true;
		m_BindingSets[set][binding].bufferInfo.buffer = static_cast<BufferVK*>(buffer)->Get();
		m_BindingSets[set][binding].bufferInfo.offset = offset;
		m_BindingSets[set][binding].bufferInfo.range = range;

		size_t hash = 0;
		hash_combine(hash, static_cast<BufferVK*>(buffer)->Get());
		//hash_combine(hash, offset);
		//hash_combine(hash, range);

		m_BindingSets[set][binding].hash = hash;
	}

	void CommandBufferVK::BindImage(Texture* texture, uint32_t set, uint32_t binding, uint32_t arrayIndex)
	{
		m_DirtySets[set] = true;
		m_BindingSets[set][binding].imageInfo.imageView = static_cast<TextureViewVK*>(texture->GetDefaultView())->Get();
		m_BindingSets[set][binding].imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		m_BindingSets[set][binding].imageInfo.sampler = static_cast<TextureVK*>(texture)->GetSampler();

		size_t hash = 0;
		hash_combine(hash, static_cast<TextureViewVK*>(texture->GetDefaultView())->Get());
		hash_combine(hash, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
		hash_combine(hash, static_cast<TextureVK*>(texture)->GetSampler());

		m_BindingSets[set][binding].hash = hash;
	}

	void CommandBufferVK::BindIndexBuffer(Buffer* buffer, uint64_t offset, uint32_t indexType)
	{
		vkCmdBindIndexBuffer(m_Handle, static_cast<BufferVK*>(buffer)->Get(), static_cast<VkDeviceSize>(offset), (VkIndexType)indexType);
	}

	void CommandBufferVK::BindPipeline(Pipeline* pipeline)
	{
		m_Pipeline = static_cast<PipelineVK*>(pipeline);
		vkCmdBindPipeline(m_Handle, static_cast<VkPipelineBindPoint>(pipeline->GetBindPoint()), m_Pipeline->Get());
	}

	void CommandBufferVK::BindVertexBuffer(uint32_t start, uint32_t num, std::vector<Buffer*> buffers, std::vector<uint64_t> offsets)
	{
		std::vector<VkBuffer> bufs = {};
		for (auto buffer : buffers)
			bufs.emplace_back(static_cast<BufferVK*>(buffer)->Get());
		vkCmdBindVertexBuffers(m_Handle, start, num, bufs.data(), offsets.data());
	}

	void CommandBufferVK::Draw(uint32_t vertexCount, uint32_t instanceCount, uint32_t firstVertex, uint32_t firstInstance)
	{
		// flush pipeline, push constants, descriptor sets
		FlushDescriptorSets();

		vkCmdDraw(m_Handle, vertexCount, instanceCount, firstVertex, firstInstance);
	}

	void CommandBufferVK::DrawIndexed(uint32_t indexCount, uint32_t instanceCount, uint32_t firstIndex, uint32_t vertexOffset, uint32_t firstInstance)
	{
		FlushDescriptorSets();

		vkCmdDrawIndexed(m_Handle, indexCount, instanceCount, firstIndex, vertexOffset, firstInstance);
	}

	void CommandBufferVK::DrawIndexedIndirect(Buffer* buffer, uint64_t offset, uint32_t drawCount, uint32_t stride)
	{
		FlushDescriptorSets();

		vkCmdDrawIndexedIndirect(m_Handle, static_cast<BufferVK*>(buffer)->Get(), offset, drawCount, stride);
	}

	void CommandBufferVK::DrawIndirect(Buffer* buffer, uint64_t offset, uint32_t drawCount, uint32_t stride)
	{
		FlushDescriptorSets();

		vkCmdDrawIndirect(m_Handle, static_cast<BufferVK*>(buffer)->Get(), offset, drawCount, stride);
	}

	void CommandBufferVK::End()
	{
		vkEndCommandBuffer(m_Handle);
	}

	void CommandBufferVK::EndRenderPass()
	{
		vkCmdEndRenderPass(m_Handle);
	}

	void CommandBufferVK::SetPushConstant(ShaderType type, uint32_t offset, uint32_t size, void* data)
	{
		vkCmdPushConstants(m_Handle, m_Pipeline->GetShaderProgram()->GetPipelineLayout(), (uint32_t)type, offset, size, data);
	}

	void CommandBufferVK::SetScissors(uint32_t first, uint32_t count, std::vector<Rect2D> scissors)
	{
		std::vector<VkRect2D> recs = {};
		for (auto& s : scissors)
			recs.push_back({ {s.offset.x, s.offset.y}, { s.extent.x, s.extent.y } });
		vkCmdSetScissor(m_Handle, first, count, recs.data());
	}

	void CommandBufferVK::SetViewports(uint32_t first, uint32_t count, std::vector<Viewport> viewports)
	{
		std::vector<VkViewport> vkViewports = {};
		for (auto& vp : viewports)
			vkViewports.push_back({ vp.x, vp.y, vp.width, vp.height, vp.minDepth, vp.maxDepth });
		vkCmdSetViewport(m_Handle, first, count, vkViewports.data());
	}

	void CommandBufferVK::FlushDescriptorSets()
	{
		for (auto& [set, dirty] : m_DirtySets) {

			size_t hash = 0;
			for (auto& [binding, resource] : m_BindingSets[set]) {
				hash_combine(hash, binding);
				hash_combine(hash, resource.hash);
			}

			auto& layout = m_Pipeline->GetShaderProgram()->GetSetLayout(set);
			auto& [cached, descriptor] = m_DescriptorPool->RequestDescriptorSet(layout, hash);

			if (!cached)
			{
				auto& updateTemplate = m_Pipeline->GetShaderProgram()->GetUpdateTemplate(set);
				std::vector<BindingInfo> bindings;
				for (auto it = m_BindingSets[set].begin(); it != m_BindingSets[set].end(); ++it)
					bindings.push_back(it->second);
				vkUpdateDescriptorSetWithTemplate(m_Device->Get(), descriptor, updateTemplate, bindings.data());
			}

			vkCmdBindDescriptorSets(m_Handle, UtilsVK::PipelineBindPointToVk(m_Pipeline->GetBindPoint()),
				m_Pipeline->GetShaderProgram()->GetPipelineLayout(), set, 1, &descriptor, 0, nullptr);
		}
	}
}
