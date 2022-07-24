#pragma once

#include "Pipeline.h"
#include "Shader.h"
#include "Swapchain.h"

enum class API {
	None = 0, OpenGL = 1, Vulkan = 2, DirectX = 3
};

struct GLFWwindow;
class GraphicsContext;
class GraphicsDevice 
{
public:
	virtual ~GraphicsDevice() {}
	
	virtual void WaitForIdle() = 0;

	virtual GraphicsContext* CreateGraphicsContext(Swapchain* swap) = 0;
	virtual Buffer* CreateBuffer(const BufferDesc& desc, void* data) = 0;
	virtual Texture* CreateTexture(const TextureDesc& desc, const unsigned char* data) = 0;
	virtual RenderPass* CreateRenderPass(const RenderPassDesc& desc) = 0;
	virtual Pipeline* CreateGraphicsPipeline(const GraphicsPipelineDesc& desc, RenderPass* renderpass) = 0;
	virtual Pipeline* CreateComputePipeline(const ComputePipelineDesc& desc) = 0;
	virtual Swapchain* CreateSwapchain(const SwapchainDesc& desc, GLFWwindow* window) = 0;
	virtual Shader* CreateShader(const ShaderDesc& desc) = 0;
	virtual ShaderProgram* CreateShaderProgram(std::vector<Shader*> shaders) = 0;

	static GraphicsDevice* Create(GLFWwindow* window, API api);
	inline static API GetAPI() { return s_API; }

private:
	static API s_API;
};
