#pragma once

#include <GLFW/glfw3.h>

#include "CommandBuffer.h"
#include "Framebuffer.h"

enum class API {
	None = 0, OpenGL = 1, Vulkan = 2, DirectX = 3
};

class GraphicsDevice 
{
public:
	virtual void Submit(CmdList* cb) = 0;
	virtual void SwapBuffers() const = 0;

	Framebuffer* GetSwapchainFramebuffer() { return m_SwapFB; }

	static GraphicsDevice* Create(GLFWwindow* window);
private:
	Framebuffer* m_SwapFB;
	static API s_API;
};