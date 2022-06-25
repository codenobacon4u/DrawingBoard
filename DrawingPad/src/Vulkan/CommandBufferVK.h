#pragma once
#include "CommandBuffer.h"

#include <vulkan/vulkan.h>

#include "DescriptorSetVK.h"
#include "PipelineVK.h"

namespace Vulkan
{
	struct BindingInfoVK {
		VkDescriptorBufferInfo bufferInfo;
		VkDescriptorImageInfo imageInfo;
		size_t hash;
	};

	class GraphicsDeviceVK;
	class CommandBufferVK : public CommandBuffer
	{
	public:
		CommandBufferVK(GraphicsDeviceVK* device, VkCommandPool pool, VkCommandBufferLevel level);
		~CommandBufferVK();

		virtual void Begin() override;
		virtual void BeginRenderPass(RenderPass* renderpass, std::vector<TextureView*> renderTargets, std::vector<ClearValue> clearValues) override;
		virtual void BindBuffer(Buffer* buffer, uint64_t offset, uint64_t range, uint32_t set, uint32_t binding, uint32_t arrayIndex = 0) override;
		virtual void BindImage(Texture* texture, uint32_t set, uint32_t binding, uint32_t arrayIndex = 0) override;
		virtual void BindIndexBuffer(Buffer* buffer, uint64_t offset) override;
		virtual void BindPipeline(Pipeline* pipeline) override;
		virtual void BindVertexBuffer(uint32_t start, uint32_t num, std::vector<Buffer*> buffers, std::vector<uint64_t> offsets) override;
		virtual void Draw(uint32_t vertexCount, uint32_t instanceCount, uint32_t firstVertex, uint32_t firstInstance) override;
		virtual void DrawIndexed(uint32_t indexCount, uint32_t instanceCount, uint32_t firstIndex, uint32_t vertexOffset, uint32_t firstInstance) override;
		virtual void DrawIndexedIndirect(Buffer* buffer, uint64_t offset, uint32_t drawCount, uint32_t stride) override;
		virtual void DrawIndirect(Buffer* buffer, uint64_t offset, uint32_t drawCount, uint32_t stride) override;
		virtual void End() override;
		virtual void EndRenderPass() override;
		virtual void SetScissors(uint32_t first, uint32_t count, std::vector<Rect2D> scissors) override;
		virtual void SetViewports(uint32_t first, uint32_t count, std::vector<Viewport> viewports) override;

		void FlushDescriptorSets();

		VkCommandBuffer Get() const { return m_Curr; }

	private:
		GraphicsDeviceVK* m_Device;
		VkCommandPool m_Pool = VK_NULL_HANDLE;
		VkCommandBuffer m_Curr = VK_NULL_HANDLE;

		PipelineVK* m_Pipeline = nullptr;

		DescriptorSetPoolVK* m_DescriptorPool;
		std::map<uint32_t, bool> m_DirtySets;
		std::map<uint32_t, std::map<uint32_t, BindingInfoVK>> m_BindingSets;
	};
}
