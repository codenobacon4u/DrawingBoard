#include "pwpch.h"
#include "GraphicsDevice.h"
#include "OpenGL/OpenGLGraphicsDevice.h"

API GraphicsDevice::s_API = API::OpenGL;

GraphicsDevice* GraphicsDevice::Create(GLFWwindow* window)
{
	switch (s_API)
	{
    case API::None:
        break;
    case API::OpenGL:
        return new OpenGLGraphicsDevice(window);
        break;
    case API::Vulkan:
        return new VkGraphicsDevice(window);
        break;
    case API::DirectX:
        break;
    default:
        break;
	}
}