#pragma once

#include "GraphicsDevice.h"
#include <Vulkan/vulkan.h>

class VkGraphicsDevice : public GraphicsDevice 
{
public:
	VkGraphicsDevice(GLFWwindow* window);
private:
	void CreateInstance();
	std::vector<const char*> InitInstanceExtensions(std::vector<const char*> extns);
	std::vector<const char*> InitInstanceLayers(std::vector<const char*> lyrs);
private:
	VkInstance* m_Instance;
	VkPhysicalDevice* m_PhysicalDevice;
	VkDevice* m_Device;
};