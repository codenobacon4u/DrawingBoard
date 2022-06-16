#include "pwpch.h"
#include "GraphicsDevice.h"

#include "Vulkan/GraphicsDeviceVK.h"

API GraphicsDevice::s_API = API::Vulkan;

GraphicsDevice* GraphicsDevice::Create(GLFWwindow* window, API api)
{
    s_API = api;
	switch (api)
	{
    case API::None:
        return nullptr;
        break;
    case API::OpenGL:
        return nullptr;
        break;
    case API::Vulkan:
        return DBG_NEW Vulkan::GraphicsDeviceVK();
        break;
    case API::DirectX:
        return nullptr;
        break;
    default:
        return nullptr;
        break;
	}
}
