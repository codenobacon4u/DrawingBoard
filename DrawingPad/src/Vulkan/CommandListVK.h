#if 0
#pragma once
#include "CommandList.h"
#include "CommandPoolVK.h"
#include "CommandBufferVK.h"

namespace VkAPI
{
	class GraphicsDeviceVK;
	class CommandListVK : public CommandList
	{
	public:
		CommandListVK(GraphicsDeviceVK* device);

		virtual void Flush() override;

		virtual void BeginRenderPass(const BeginRenderPassAttribs& attribs) override;
		virtual void EndRenderPass() override;

		virtual void ClearColor(TextureView* tv, float* color) override;
		virtual void ClearDepth(TextureView* tv, ClearDepthStencil clearFlags, float depth, uint8_t stencil) override;

		virtual void SetViewports(uint32_t num, const Viewport* viewports, uint32_t width, uint32_t height) override;
		virtual void SetScissors(uint32_t num, const Viewport* viewports, uint32_t width, uint32_t height) override;
		virtual void SetRenderTargets(uint32_t numTargsts, TextureView** targets, TextureView* depthStencil) override;
		virtual void SetPipeline(Pipeline* pipeline) override;
		virtual void SetVertexBuffers(uint32_t start, uint32_t num, Buffer** buffers, const uint32_t* offsets) override;
		virtual void SetIndexBuffer(Buffer* buffer, uint32_t offset) override;


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

		virtual void End() override;

	private:
		void VerifyCommandBuffer();
		void DisposeCurrentBuffer();

	private:
		GraphicsDeviceVK* m_vkDevice;
		CommandPoolVK* m_CmdPool = nullptr;
		CommandBufferVK m_CommandBuffer;

		VkRenderPass m_vkRenderPass = VK_NULL_HANDLE;
		VkFramebuffer m_vkFramebuffer = VK_NULL_HANDLE;

		VkSemaphore m_Available = VK_NULL_HANDLE;
	};
}
#endif