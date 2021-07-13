#include "pwpch.h"
#include "GraphicsDevice.h"

#include "OpenGL/GraphicsDeviceGL.h"
#include "Vulkan/GraphicsDeviceVK.h"
#include "Vulkan/GraphicsContextVK.h"

API GraphicsDevice::s_API = API::OpenGL;

void GraphicsDevice::SwapBuffers()
{
	if (m_MainSwap == nullptr) {
		throw std::runtime_error("Main Swapchain was not created");
	}

	SwapBuffers(m_MainSwap);
}

GraphicsContext* GraphicsDevice::CreateContext(const GraphicsContextDesc& desc)
{
    m_ImmediateContexts.resize(m_ImmediateContexts.size() + 1);
    switch (s_API)
    {
    case API::None:
        return nullptr;
        break;
    case API::OpenGL:
        return nullptr;
        break;
    case API::Vulkan:
        m_ImmediateContexts.push_back(new VkAPI::GraphicsContextVK((VkAPI::GraphicsDeviceVK*)this, desc));
        return m_ImmediateContexts.back();
        break;
    case API::DirectX:
        return nullptr;
        break;
    default:
        return nullptr;
        break;
    }
}

GraphicsDevice* GraphicsDevice::Create(GLFWwindow* window, API api)
{
    s_API = api;
	switch (api)
	{
    case API::None:
        return nullptr;
        break;
    case API::OpenGL:
        return new GlAPI::GraphicsDeviceGL(window);
        break;
    case API::Vulkan:
        return new VkAPI::GraphicsDeviceVK();
        break;
    case API::DirectX:
        return nullptr;
        break;
    default:
        return nullptr;
        break;
	}
}