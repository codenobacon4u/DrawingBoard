#include "pwpch.h"
#include "VkGraphicsDevice.h"

VkGraphicsDevice::VkGraphicsDevice(GLFWwindow* window)
{
	if (!glfwVulkanSupported())
		throw std::runtime_error("Vulkan is not supported!");

	CreateInstance();
}

void VkGraphicsDevice::CreateInstance() {
	std::vector<const char*> extensions = InitInstanceExtensions(std::vector<const char*>());
	std::vector<const char*> layers = InitInstanceLayers(std::vector<const char*>());

	VkInstanceCreateInfo instInfo;
	VkApplicationInfo appInfo;
	appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	appInfo.apiVersion = VK_MAKE_VERSION(1, 1, 0);
	appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
	appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
	appInfo.pApplicationName = "DrawingPad-Test";
	appInfo.pEngineName = "DrawingPad";

	instInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	instInfo.pApplicationInfo = &appInfo;

	instInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
	instInfo.ppEnabledExtensionNames = extensions.data();

	instInfo.enabledLayerCount = static_cast<uint32_t>(layers.size());
	instInfo.ppEnabledLayerNames = layers.data();

	vkCreateInstance(&instInfo, nullptr, m_Instance);
}

std::vector<const char*> VkGraphicsDevice::InitInstanceExtensions(std::vector<const char*> extns)
{
	std::vector<const char*> res;
	uint32_t glfwExtensionCount = 0;
	const char** glfwExtensions;
	glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);
	for (uint32_t i = 0; i < glfwExtensionCount; i++) {
		res.emplace_back(glfwExtensions[i]);
	}
	if (!extns.empty()) {
		//Prepare required extensions
		uint32_t count;
		vkEnumerateInstanceExtensionProperties(nullptr, &count, nullptr);
		std::vector<VkExtensionProperties> props(count);
		vkEnumerateInstanceExtensionProperties(nullptr, &count, props.data());
		for (const auto& ext : extns) {
			bool found = false;
			for (const auto& prop : props) {
				if (strcmp(ext, prop.extensionName) == 0) {
					found = true;
					break;
				}
			}
			if (!found)
				std::runtime_error("Vulkan extension not found");
			res.emplace_back(ext);
		}
	}
	return res;
}

std::vector<const char*> VkGraphicsDevice::InitInstanceLayers(std::vector<const char*> lyrs)
{
	std::vector<const char*> res;
	if (!lyrs.empty() && (lyrs.size() != 0)) {
		uint32_t count;
		vkEnumerateInstanceLayerProperties(&count, nullptr);
		std::vector<VkLayerProperties> props(count);
		vkEnumerateInstanceLayerProperties(&count, props.data());
		for (const char* name : lyrs) {
			bool found = false;
			for (const auto& lp : props) {
				if (strcmp(name, lp.layerName) == 0) {
					found = true;
					break;
				}
			}
			if (!found)
				std::runtime_error("Vulkan validation layer not found");
			res.emplace_back(name);
		}
	}
	else {
		res.emplace_back("VK_LAYER_LUNARG_standard_validation");
	}
	return res;
}
