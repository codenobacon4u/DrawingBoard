#pragma once
#include "GraphicsContext.h"
#include "GraphicsDeviceVK.h"
#include "CommandPoolVK.h"
#include "CommandBufferVK.h"

namespace VkAPI
{
	struct ShaderResourceVK {
		VkDescriptorBufferInfo Buffer = {};
		VkDescriptorImageInfo Texture = {};
	};

	struct ResourceBindingsVK {
		ShaderResourceVK Sets[4][32] = {};
		DSLKey SetLayouts[4] = {};
		uint32_t SetCount = 0;
	};

	class GraphicsContextVK : public GraphicsContext
	{
	public:
		GraphicsContextVK(GraphicsDeviceVK* device, const GraphicsContextDesc& desc);
		
		~GraphicsContextVK();

		virtual void Flush() override;

		virtual void Begin(uint32_t frameIdx) override;
		virtual void BeginRenderPass(const BeginRenderPassAttribs& attribs) override;
		virtual void EndRenderPass() override;

		virtual void ClearColor(TextureView* tv, const float* color) override;
		virtual void ClearDepth(TextureView* tv, ClearDepthStencil clearFlags, float depth, uint8_t stencil) override;

		virtual void SetViewports(uint32_t num, const Viewport* viewports, uint32_t width, uint32_t height) override;
		virtual void SetScissors(uint32_t num, int32_t offX, int32_t offY, uint32_t width, uint32_t height) override;
		virtual void SetRenderTargets(uint32_t numTargets, TextureView** targets, TextureView* depthStencil) override;
		virtual void SetPipeline(Pipeline* pipeline) override;
		virtual void SetVertexBuffers(uint32_t start, uint32_t num, Buffer** buffers, const uint32_t* offset) override;
		virtual void SetIndexBuffer(Buffer* buffer, uint32_t offset) override;
		virtual void SetShaderResource(ResourceBindingType type, uint32_t set, uint32_t binding, Buffer* buffer) override;
		virtual void SetShaderResource(ResourceBindingType type, uint32_t set, uint32_t binding, Texture* buffer) override;

		virtual void Draw(const DrawAttribs& attribs) override;
		virtual void DrawIndexed(const DrawAttribs& attribs) override;
		virtual void DrawIndirect(const DrawIndirectAttribs& attribs) override;
		virtual void DrawIndexedIndirect(const DrawIndirectAttribs& attribs) override;
		//TODO: Look into drawing meshes
		//virtual void DrawMesh() override;
		//virtual void DrawMeshIndirect() override;
		//virtual void DrawMeshIndirectCount() override;

		virtual void Dispatch(const DispatchAttribs& attribs) override;
		virtual void DispatchIndirect(const DispatchIndirectAttribs& attribs) override;

		virtual void ResourceBarrier() override;
		virtual void UploadBuffer(Buffer* src, uint32_t offset, size_t size, void* data) override;
		virtual void CopyBuffer(Buffer* src, Buffer* dst, uint32_t size) override;
		virtual void UploadTexture() override;

		void AddWaitSemaphore(VkSemaphore* semaphore) {
			m_WaitSemaphores.push_back(*semaphore);
		}

		void AddSignalSemaphore(VkSemaphore* semaphore) {
			m_SignalSemaphores.push_back(*semaphore);
		}

		void AddSubFence(VkFence* fence) {
			m_SubFence = fence;
		}

	private:
		void PrepareDraw();
		void BindDescriptorSets();
		void UpdateDescriptorSet(VkDescriptorSet set, const DSLKey& key, ShaderResourceVK* bindings);

		void VerifyCommandBuffer();
		void TransitionTexture(TextureVK* tex, ViewType oldState, ViewType newState);
		void DisposeCurrentBuffer();

	private:
		GraphicsDeviceVK* m_vkDevice;
		CommandPoolVK* m_CmdPool[3];
		DescriptorSetPoolVK* m_DescPool[3];
		CommandBufferVK m_CommandBuffer;

		VkRenderPass m_vkRenderPass = VK_NULL_HANDLE;
		VkFramebuffer m_vkFramebuffer = VK_NULL_HANDLE;

		ResourceBindingsVK m_Bindings;
		uint32_t m_UpdateSetsMask = 0;
		uint32_t m_UpdateSetsDynamicMask = 0;

		std::vector<VkSemaphore> m_WaitSemaphores;
		std::vector<VkSemaphore> m_SignalSemaphores;
		VkFence* m_SubFence = nullptr;

		uint32_t m_FrameIndex = 0;
	};
}