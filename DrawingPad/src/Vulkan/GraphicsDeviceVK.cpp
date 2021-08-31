#include "pwpch.h"
#include "GraphicsDeviceVK.h"

#include <set>

#include "SwapchainVK.h"
#include "TextureVK.h"
#include "BufferVK.h"
#include "ShaderVK.h"
#include "PipelineVK.h"
#include "TextureManagerVK.h"

namespace VkAPI
{
#ifdef PW_DEBUG
	const bool enableValidation = true;
#else
	const bool enableValidation = false;
#endif

	const std::vector<const char*> deviceExtensions = {
		VK_KHR_SWAPCHAIN_EXTENSION_NAME,
		//VK_KHR_SHADER_NON_SEMANTIC_INFO_EXTENSION_NAME
	};

	static PFN_vkCreateDebugUtilsMessengerEXT  CreateDebugUtilsMessengerEXT = nullptr;
	static PFN_vkDestroyDebugUtilsMessengerEXT DestroyDebugUtilsMessengerEXT = nullptr;

	VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageType, const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void* pUserData) {
		std::cerr << "validation layer: " << pCallbackData->pMessage << std::endl;
		UtilsVK::Log("validation_layers.log", std::string(pCallbackData->pMessage));
		return VK_FALSE;
	}

	GraphicsDeviceVK::GraphicsDeviceVK()
	{
		if (!glfwVulkanSupported())
			throw std::runtime_error("Vulkan is not supported!");

		CreateInstance();
		CreatePhysicalDevice();
		CreateDevice();
		if (enableValidation)
			DebugMarker::Setup(m_Device, m_PhysicalDevice);
		CreateDescriptiorPool();
		CreateCommandPool();
		m_FramebufferPool = DBG_NEW FramebufferPoolVK(*this);
		m_RenderPassPool = DBG_NEW RenderPassPoolVK(*this);
		m_DescriptorSetPool = DBG_NEW DescriptorSetPoolVK(this);
		m_TempPool = DBG_NEW CommandPoolVK(this, m_GraphicsIndex, 0);
		m_TextureManager = DBG_NEW TextureManagerVK(this);
	}

	GraphicsDeviceVK::~GraphicsDeviceVK()
	{
		std::cout << "DELETING GRAPHICS DEVICE" << std::endl;
		vkDeviceWaitIdle(m_Device);

		delete m_FramebufferPool;
		delete m_RenderPassPool;
		delete m_DescriptorSetPool;
		delete m_TempPool;

		vkDestroyDevice(m_Device, nullptr);
		if (enableValidation)
			DestroyDebugUtilsMessengerEXT(m_Instance, m_DebugMessenger, nullptr);

		vkDestroyInstance(m_Instance, nullptr);
	}

	void GraphicsDeviceVK::SubmitCommandBuffer(const VkSubmitInfo& info, VkFence* fence)
	{
		auto result = vkQueueSubmit(m_GraphicsQueue, 1, &info, *fence);
		if (result != VK_SUCCESS)
		{
			throw std::runtime_error("Failed to submit commandbuffer!");
		}
	}
	
	void GraphicsDeviceVK::WaitForIdle()
	{
		vkDeviceWaitIdle(m_Device);
	}

	void GraphicsDeviceVK::Present()
	{
	}

	Buffer* GraphicsDeviceVK::CreateBuffer(const BufferDesc& desc, void* data)
	{
		return DBG_NEW BufferVK(this, desc, data);
	}

	Texture* GraphicsDeviceVK::CreateTexture(const TextureDesc& desc, const unsigned char* data)
	{
		return DBG_NEW TextureVK(this, desc, data);
	}

	TextureVK* GraphicsDeviceVK::CreateTextureFromImage(const TextureDesc& desc, VkImage img)
	{
		return DBG_NEW TextureVK(this, desc, std::move(img));
	}

	uint32_t GraphicsDeviceVK::FindMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties)
	{
		VkPhysicalDeviceMemoryProperties memProps;
		vkGetPhysicalDeviceMemoryProperties(m_PhysicalDevice, &memProps);

		for (uint32_t i = 0; i < memProps.memoryTypeCount; i++)
			if ((typeFilter & (1 << i)) && (memProps.memoryTypes[i].propertyFlags & properties) == properties)
				return i;

		throw std::runtime_error("Failed to find suitable memory type!");
	}

	RenderPassVK* GraphicsDeviceVK::CreateRenderPass(const RenderPassDesc& desc)
	{
		return DBG_NEW RenderPassVK(this, desc);
	}

	Framebuffer* GraphicsDeviceVK::CreateFramebuffer(const FramebufferDesc& desc)
	{
		return DBG_NEW FramebufferVK(this, desc);
	}

	Pipeline* GraphicsDeviceVK::CreateGraphicsPipeline(const GraphicsPipelineDesc& desc)
	{
		return DBG_NEW PipelineVK(this, desc);
	}

	Pipeline* GraphicsDeviceVK::CreateComputePipeline(const ComputePipelineDesc& desc)
	{
		return DBG_NEW PipelineVK(this, desc);
	}

	Swapchain* GraphicsDeviceVK::CreateSwapchain(const SwapchainDesc& desc, GraphicsContext* context, GLFWwindow* window)
	{
		return DBG_NEW SwapchainVK(this, (GraphicsContextVK*)context, desc, window);
	}

	Shader* GraphicsDeviceVK::CreateShader(const ShaderDesc& desc)
	{
		return DBG_NEW ShaderVK(this, desc);
	}
	
	QueueFamilyIndices GraphicsDeviceVK::FindQueueFamilies(VkQueueFlags flags)
	{
		QueueFamilyIndices indices;

		uint32_t queueCount;
		vkGetPhysicalDeviceQueueFamilyProperties(m_PhysicalDevice, &queueCount, nullptr);
		std::vector<VkQueueFamilyProperties> queueFamilies(queueCount);
		vkGetPhysicalDeviceQueueFamilyProperties(m_PhysicalDevice, &queueCount, queueFamilies.data());

		uint32_t i = 0;
		for (const auto& queueFamily : queueFamilies) {
			if (queueFamily.queueCount > 0 && (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) && (flags & VK_QUEUE_GRAPHICS_BIT)) {
				m_GraphicsIndex = i;
				indices.graphicsFamily = i;
			}
			if (queueFamily.queueCount > 0 && (queueFamily.queueFlags & VK_QUEUE_COMPUTE_BIT) && (flags & VK_QUEUE_COMPUTE_BIT)) {
				m_ComputeIndex = i;
				indices.computeFamily = i;
			}
			if (indices.isComplete()) {
				break;
			}
			i++;
		}
		return indices;
	}

	bool GraphicsDeviceVK::QueryPresentSupport(uint32_t index, VkSurfaceKHR surface)
	{
		VkBool32 support = VK_FALSE;
		vkGetPhysicalDeviceSurfaceSupportKHR(m_PhysicalDevice, index, surface, &support);
		return support;
	}
	
	void GraphicsDeviceVK::CreateInstance()
	{
		VkValidationFeatureEnableEXT enables[] = { VK_VALIDATION_FEATURE_ENABLE_BEST_PRACTICES_EXT };
		VkValidationFeaturesEXT features = {};
		features.sType = VK_STRUCTURE_TYPE_VALIDATION_FEATURES_EXT;
		if (enableValidation)
		{
			features.enabledValidationFeatureCount = 1;
			features.pEnabledValidationFeatures = enables;
		}

		VkInstanceCreateInfo instInfo = {};
		VkApplicationInfo appInfo = {};
		appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
		appInfo.apiVersion = VK_MAKE_VERSION(1, 1, 0);
		appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
		appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
		appInfo.pApplicationName = "DrawingPad-Test";
		appInfo.pEngineName = "DrawingPad";

		instInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
		instInfo.pNext = &features;
		instInfo.pApplicationInfo = &appInfo;

		instInfo.enabledExtensionCount = static_cast<uint32_t>(m_EnabledExtensions.size());
		instInfo.ppEnabledExtensionNames = m_EnabledExtensions.data();

		VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo = {};
		if (enableValidation) {
			instInfo.enabledLayerCount = static_cast<uint32_t>(m_EnabledLayers.size());
			instInfo.ppEnabledLayerNames = m_EnabledLayers.data();

			debugCreateInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
			debugCreateInfo.flags = 0;
			debugCreateInfo.pNext = nullptr;
			debugCreateInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT | /*VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |*/ VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
			debugCreateInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
			debugCreateInfo.pfnUserCallback = debugCallback;
			debugCreateInfo.pUserData = nullptr;
		}
		else {
			instInfo.enabledLayerCount = 0;
			instInfo.ppEnabledLayerNames = nullptr;
		}

		if (vkCreateInstance(&instInfo, nullptr, &m_Instance) != VK_SUCCESS) {
			throw std::runtime_error("failed to create instance");
		}

		if (!enableValidation) return;
		CreateDebugUtilsMessengerEXT = reinterpret_cast<PFN_vkCreateDebugUtilsMessengerEXT>(vkGetInstanceProcAddr(m_Instance, "vkCreateDebugUtilsMessengerEXT"));
		DestroyDebugUtilsMessengerEXT = reinterpret_cast<PFN_vkDestroyDebugUtilsMessengerEXT>(vkGetInstanceProcAddr(m_Instance, "vkDestroyDebugUtilsMessengerEXT"));
		if (CreateDebugUtilsMessengerEXT(m_Instance, &debugCreateInfo, nullptr, &m_DebugMessenger) != VK_SUCCESS) {
			throw std::runtime_error("failed to set up debug messenger");
		}
	}
	
	void GraphicsDeviceVK::CreatePhysicalDevice()
	{
		uint32_t deviceCount;
		vkEnumeratePhysicalDevices(m_Instance, &deviceCount, nullptr);
		if (deviceCount == 0) {
			throw std::runtime_error("failed to find a GPU with Vulkan support");
		}

		std::vector<VkPhysicalDevice> devices(deviceCount);
		vkEnumeratePhysicalDevices(m_Instance, &deviceCount, devices.data());
		m_PhysicalDevice = devices[0];

		VkPhysicalDeviceProperties props;
		vkGetPhysicalDeviceProperties(m_PhysicalDevice, &props);
		m_Limits = props.limits;
	}
	
	void GraphicsDeviceVK::CreateDevice()
	{
		QueueFamilyIndices indices = FindQueueFamilies(VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_COMPUTE_BIT);
		//
		//std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
		//std::set<uint32_t> uniqueQueueFamilies = { indices.graphicsFamily.value(), indices.computeFamily.value() };

		const float queuePriority = 1.0f;
		//for (uint32_t queueFamily : uniqueQueueFamilies) {
		//	VkDeviceQueueCreateInfo queueCreateInfo;
		//	queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		//	queueCreateInfo.queueFamilyIndex = queueFamily;
		//	queueCreateInfo.queueCount = 1;
		//	queueCreateInfo.pQueuePriorities = &queuePriority;
		//	queueCreateInfos.push_back(queueCreateInfo);
		//}
		VkDeviceQueueCreateInfo queueCreateInfo = {};
		queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		queueCreateInfo.flags = 0;
		queueCreateInfo.queueFamilyIndex = indices.graphicsFamily.value();
		queueCreateInfo.queueCount = 1;
		queueCreateInfo.pQueuePriorities = &queuePriority;
		VkPhysicalDeviceFeatures physicalFeatures;
		vkGetPhysicalDeviceFeatures(m_PhysicalDevice, &physicalFeatures);
		VkPhysicalDeviceFeatures deviceFeatures = {};
		VkDeviceCreateInfo createInfo = {};
		createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
		createInfo.flags = 0;
		createInfo.queueCreateInfoCount = 1;//static_cast<uint32_t>(queueCreateInfos.size());
		createInfo.pQueueCreateInfos = &queueCreateInfo;//queueCreateInfos.data();
		createInfo.pEnabledFeatures = &deviceFeatures;
		createInfo.enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size());
		createInfo.ppEnabledExtensionNames = deviceExtensions.data();
		createInfo.enabledLayerCount = 0;
		createInfo.ppEnabledLayerNames = nullptr;
		if (vkCreateDevice(m_PhysicalDevice, &createInfo, nullptr, &m_Device) != VK_SUCCESS) {
			throw std::runtime_error("failed to create logical device");
		}

		vkGetDeviceQueue(m_Device, indices.graphicsFamily.value(), 0, &m_GraphicsQueue);
	}
	
	void GraphicsDeviceVK::CreateDescriptiorPool()
	{
	}
	
	void GraphicsDeviceVK::CreateCommandPool()
	{
		//VkCommandPoolCreateInfo createInfo;
		//createInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
		//createInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
		//createInfo.queueFamilyIndex = m_GraphicsIndex;
		//if (vkCreateCommandPool(m_Device, &createInfo, nullptr, &m_CommandPool) != VK_SUCCESS)
		//	throw std::runtime_error("Unable to create command pool");
	}
	
	VkFence GraphicsDeviceVK::GetNextSubFence()
	{
		if (!m_SubFences.empty()) {
			VkFence res = m_SubFences.back();
			m_SubFences.pop_back();
			return res;
		}
		else {
			VkFence fence;
			VkFenceCreateInfo createInfo = {};
			createInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
			if (vkCreateFence(m_Device, &createInfo, nullptr, &fence))
				throw std::runtime_error("Failed to create submission fence");
			return fence;
		}
	}

	bool GraphicsDeviceVK::IsExtensionAvailable(const char* extName) const
	{
		for (const auto& ext : m_ExtensionProps)
			if (!strcmp(ext.extensionName, extName))
				return true;
		return false;
	}

	bool GraphicsDeviceVK::IsLayerAvailable(const char* lyrName) const
	{
		for (const auto& lyr : m_LayerProps)
			if (!strcmp(lyr.layerName, lyrName))
				return true;
		return false;
	}
	
	std::vector<const char*> GraphicsDeviceVK::InitInstanceExtensions(std::vector<const char*> extns)
	{
		uint32_t extensionCount = 0;
		auto extensions = vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);
		m_ExtensionProps.resize(extensionCount);
		vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, m_ExtensionProps.data());

		std::vector<const char*> res;
		uint32_t glfwExtensionCount = 0;
		const char** glfwExtensions;
		glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);
		for (uint32_t i = 0; i < glfwExtensionCount; i++) {
			if (!IsExtensionAvailable(glfwExtensions[i]))
			{
				UtilsVK::Log("errors.log", "ERROR: Required extension not found!");
				throw std::runtime_error("Required extension not found!");
			}
			res.emplace_back(glfwExtensions[i]);
		}

		if (IsExtensionAvailable(VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME))
			res.emplace_back(VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME);

		if (enableValidation) {
			if (IsExtensionAvailable(VK_EXT_DEBUG_UTILS_EXTENSION_NAME))
				res.emplace_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
			else
			{
				UtilsVK::Log("errors.log", "ERROR: Extension VK_EXT_debug_utils not available");
				throw std::runtime_error("Extension VK_EXT_debug_utils not available");
			}
		}
		return res;
	}
	
	std::vector<const char*> GraphicsDeviceVK::InitInstanceLayers(std::vector<const char*> lyrs)
	{
		uint32_t layerCount = 0;
		auto extensions = vkEnumerateInstanceLayerProperties(&layerCount, nullptr);
		m_LayerProps.resize(layerCount);
		vkEnumerateInstanceLayerProperties(&layerCount, m_LayerProps.data());

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
				{
					UtilsVK::Log("errors.log", "ERROR: Vulkan validation layer not found");
					throw std::runtime_error("Vulkan validation layer not found");
				}
				res.emplace_back(name);
			}
		}
		else {
			if (IsLayerAvailable("VK_LAYER_KHRONOS_validation"))
				res.emplace_back("VK_LAYER_KHRONOS_validation");
			else
				UtilsVK::Log("errors.log", "ERROR: Layer VK_LAYER_KHRONOS_validation not found");
			//	throw std::runtime_error("Layer VK_LAYER_KHRONOS_validation not found");
		}
		return res;
	}
	
	void GraphicsDeviceVK::SwapBuffers(Swapchain* swapchain) const
	{
		swapchain->Present(60);
	}
}
