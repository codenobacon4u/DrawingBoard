#include "pwpch.h"
#include "GraphicsDeviceGL.h"

namespace GlAPI
{
	GraphicsDeviceGL::GraphicsDeviceGL(GLFWwindow* window)
	{
	}

	void GraphicsDeviceGL::WaitForIdle()
	{
	}

	void GraphicsDeviceGL::Present()
	{
	}

	Buffer* GraphicsDeviceGL::CreateBuffer(const BufferDesc& desc, void* data)
	{
		return nullptr;
	}
	
	Texture* GraphicsDeviceGL::CreateTexture(const TextureDesc& desc, const unsigned char* data)
	{
		return nullptr;
	}
	
	RenderPass* GraphicsDeviceGL::CreateRenderPass(const RenderPassDesc& desc)
	{
		return nullptr;
	}
	
	Framebuffer* GraphicsDeviceGL::CreateFramebuffer(const FramebufferDesc& desc)
	{
		return nullptr;
	}
	
	Pipeline* GraphicsDeviceGL::CreateGraphicsPipeline(const GraphicsPipelineDesc& desc)
	{
		return nullptr;
	}
	
	Pipeline* GraphicsDeviceGL::CreateComputePipeline(const ComputePipelineDesc& desc)
	{
		return nullptr;
	}

	Swapchain* GraphicsDeviceGL::CreateSwapchain(const SwapchainDesc& desc, GLFWwindow* window)
	{
		return nullptr;
	}

	Shader* GraphicsDeviceGL::CreateShader(const ShaderDesc& desc)
	{
		return nullptr;
	}
	
	void GraphicsDeviceGL::SwapBuffers(Swapchain* swapchain) const
	{
	}
}
