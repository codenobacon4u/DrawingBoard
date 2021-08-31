#pragma once

#include "GraphicsDevice.h"

namespace GlAPI {
	class GraphicsDeviceGL : public GraphicsDevice
	{
	public:
		GraphicsDeviceGL(GLFWwindow* window);

		//virtual void Submit(const CommandList* cb) override;
		virtual void WaitForIdle() override;
		virtual void Present() override;

		//virtual CommandList* CreateCommandList() override;
		virtual Buffer* CreateBuffer(const BufferDesc& desc, void* data) override;
		virtual Texture* CreateTexture(const TextureDesc& desc, const unsigned char* data) override;
		virtual RenderPass* CreateRenderPass(const RenderPassDesc& desc) override;
		virtual Framebuffer* CreateFramebuffer(const FramebufferDesc& desc) override;
		virtual Pipeline* CreateGraphicsPipeline(const GraphicsPipelineDesc& desc) override;
		virtual Pipeline* CreateComputePipeline(const ComputePipelineDesc& desc) override;
		virtual Swapchain* CreateSwapchain(const SwapchainDesc& desc, GraphicsContext* context, GLFWwindow* window) override;
		virtual Shader* CreateShader(const ShaderDesc& desc) override;

	protected:
		virtual void SwapBuffers(Swapchain* swapchain) const override;
	};
}