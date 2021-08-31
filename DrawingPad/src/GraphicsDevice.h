#pragma once
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <vector>

#include "GraphicsContext.h"

#include "CommandList.h"
#include "Framebuffer.h"
#include "Swapchain.h"
#include "Shader.h"
#include "TextureManager.h"

enum class API {
	None = 0, OpenGL = 1, Vulkan = 2, DirectX = 3
};

class GraphicsDevice 
{
public:
	virtual ~GraphicsDevice() {}

	void SwapBuffers();
	
	virtual void WaitForIdle() = 0;
	virtual void Present() = 0;

	virtual Buffer* CreateBuffer(const BufferDesc& desc, void* data) = 0;
	virtual Texture* CreateTexture(const TextureDesc& desc, const unsigned char* data) = 0;
	virtual RenderPass* CreateRenderPass(const RenderPassDesc& desc) = 0;
	virtual Framebuffer* CreateFramebuffer(const FramebufferDesc& desc) = 0;
	virtual Pipeline* CreateGraphicsPipeline(const GraphicsPipelineDesc& desc) = 0;
	virtual Pipeline* CreateComputePipeline(const ComputePipelineDesc& desc) = 0;
	virtual Swapchain* CreateSwapchain(const SwapchainDesc& desc, GraphicsContext* context, GLFWwindow* window) = 0;
	virtual Shader* CreateShader(const ShaderDesc& desc) = 0;

	GraphicsContext* CreateContext(const GraphicsContextDesc& desc);

	TextureManager* GetTextureManager() { return m_TextureManager; }

	static GraphicsDevice* Create(GLFWwindow* window, API api);
	inline static API GetAPI() { return s_API; }
protected:
	virtual void SwapBuffers(Swapchain* swapchain) const = 0;

	TextureManager* m_TextureManager;

private:
	static API s_API;
};