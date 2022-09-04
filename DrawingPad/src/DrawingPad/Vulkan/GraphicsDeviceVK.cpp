#include "dppch.h"
#include "GraphicsDeviceVK.h"

#include <set>

#include "BufferVK.h"
#include "DebugVK.h"
#include "FramebufferPoolVK.h"
#include "GraphicsContextVK.h"
#include "PipelineVK.h"
#include "RenderPassVK.h"
#include "ShaderVK.h"
#include "SwapchainVK.h"
#include "TextureVK.h"

namespace DrawingPad
{
	namespace Vulkan
	{
#ifdef PW_DEBUG
		const bool enableValidation = true;
#else
		const bool enableValidation = false;
#endif

		std::vector<const char*> deviceExtensions = {
			//VK_KHR_SWAPCHAIN_EXTENSION_NAME,
			//VK_KHR_SHADER_NON_SEMANTIC_INFO_EXTENSION_NAME
		};

		static PFN_vkCreateDebugUtilsMessengerEXT  CreateDebugUtilsMessengerEXT = nullptr;
		static PFN_vkDestroyDebugUtilsMessengerEXT DestroyDebugUtilsMessengerEXT = nullptr;

		VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageType, const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void* pUserData) {
			switch (messageSeverity) {
			case VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT:
				DP_TRACE(pCallbackData->pMessage);
				break;
			case VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT:
				DP_INFO(pCallbackData->pMessage);
				break;
			case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT:
				DP_WARN(pCallbackData->pMessage);
				break;
			case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT:
				DP_ERROR(pCallbackData->pMessage);
				break;
			default:
				DP_TRACE(pCallbackData->pMessage);
				break;
			}
			return VK_FALSE;
		}

		GraphicsDeviceVK::GraphicsDeviceVK(GLFWwindow* window)
			: m_MainWindow(window)
		{
			CreateInstance();

			if (!glfwVulkanSupported())
				throw std::runtime_error("Vulkan is not supported!");

			auto res = glfwCreateWindowSurface(m_Instance, window, nullptr, &m_Surface);
			if (res == VK_ERROR_INITIALIZATION_FAILED)
			{
				throw std::runtime_error("API not available");
			}
			else if (res == VK_ERROR_EXTENSION_NOT_PRESENT)
			{
				throw std::runtime_error("The extension was not available");
			}
			else if (res == VK_ERROR_NATIVE_WINDOW_IN_USE_KHR)
			{
				throw std::runtime_error("Native window is in use");
			}

			CreatePhysicalDevice();
			CreateDevice();
			if (enableValidation && 0) {
				DebugMarker::Setup(m_Device, m_PhysicalDevice);
			}
			m_TempPool = DBG_NEW CommandPoolVK(this, 0, 0);
			m_FramebufferPool = DBG_NEW FramebufferPoolVK(this);
		}

		GraphicsDeviceVK::~GraphicsDeviceVK()
		{
			vkDeviceWaitIdle(m_Device);
			delete m_TempPool;
			delete m_FramebufferPool;

			if (m_MemAllocator != VK_NULL_HANDLE)
			{
				char* memoryInfo;
				vmaBuildStatsString(m_MemAllocator, &memoryInfo, VK_TRUE);
				std::cout << memoryInfo << "\n";
				vmaFreeStatsString(m_MemAllocator, memoryInfo);
				vmaDestroyAllocator(m_MemAllocator);
			}

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
				DP_ERROR("Failed to submit commandbuffer!");
				throw std::runtime_error("Failed to submit commandbuffer!");
			}
		}

		void GraphicsDeviceVK::WaitForIdle()
		{
			vkDeviceWaitIdle(m_Device);
		}

		GraphicsContext* GraphicsDeviceVK::CreateGraphicsContext(Swapchain* swap)
		{
			return DBG_NEW GraphicsContextVK(this, swap);
		}

		Buffer* GraphicsDeviceVK::CreateBuffer(const BufferDesc& desc, void* data)
		{
			return DBG_NEW BufferVK(this, desc, reinterpret_cast<uint8_t*>(data));
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

			DP_ERROR("Failed to find suitable memory type!");
			throw std::runtime_error("Failed to find suitable memory type!");
		}

		RenderPass* GraphicsDeviceVK::CreateRenderPass(const RenderPassDesc& desc)
		{
			return DBG_NEW RenderPassVK(this, desc);
		}

		Pipeline* GraphicsDeviceVK::CreateGraphicsPipeline(const GraphicsPipelineDesc& desc, RenderPass* renderpass)
		{
			return DBG_NEW PipelineVK(this, desc, renderpass);
		}

		Pipeline* GraphicsDeviceVK::CreateComputePipeline(const ComputePipelineDesc& desc)
		{
			return DBG_NEW PipelineVK(this, desc);
		}

		Swapchain* GraphicsDeviceVK::CreateSwapchain(const SwapchainDesc& desc, GLFWwindow* window)
		{
			VkSurfaceKHR surface;
			if (window != m_MainWindow)
				glfwCreateWindowSurface(m_Instance, window, nullptr, &surface);
			else
				surface = m_Surface;

			return DBG_NEW SwapchainVK(this, desc, surface);
		}

		Shader* GraphicsDeviceVK::CreateShader(const ShaderDesc& desc)
		{
			return DBG_NEW ShaderVK(this, desc);
		}

		ShaderProgram* GraphicsDeviceVK::CreateShaderProgram(std::vector<Shader*> shaders)
		{
			std::vector<ShaderVK*> transformed = {};
			for (auto* shader : shaders)
				transformed.emplace_back(static_cast<ShaderVK*>(shader));
			return DBG_NEW ShaderProgramVK(this, transformed);
		}

		const uint32_t GraphicsDeviceVK::GetGraphicsIndex()
		{
			if (m_GraphicsIndex != ~0U)
				return m_GraphicsIndex;

			for (auto i = 0; i < m_Queues.size(); i++) {
				auto const& queue = m_Queues[i][0];
				if (queue.presentSupport && queue.properties.queueCount > 0)
				{
					m_GraphicsIndex = i;
					m_GraphicsQueue = queue.queue;
					return m_GraphicsIndex;
				}
			}

			return GetQueueByFlags(VK_QUEUE_GRAPHICS_BIT, 0).familyIndex;
		}

		const VkQueue GraphicsDeviceVK::GetGraphicsQueue()
		{
			if (m_GraphicsQueue != VK_NULL_HANDLE)
				return m_GraphicsQueue;

			for (auto i = 0; i < m_Queues.size(); i++) {
				auto const& queue = m_Queues[i][0];
				if (queue.presentSupport && queue.properties.queueCount > 0)
				{
					m_GraphicsIndex = i;
					m_GraphicsQueue = queue.queue;
					return m_GraphicsQueue;
				}
			}

			return GetQueueByFlags(VK_QUEUE_GRAPHICS_BIT, 0).queue;
		}

		const Queue GraphicsDeviceVK::GetQueueByFlags(VkQueueFlags flags, uint32_t index, VkSurfaceKHR surface)
		{
			for (auto i = 0; i < m_Queues.size(); i++) {
				auto const& queue = m_Queues[i][0];
				if ((queue.properties.queueFlags & flags) && queue.properties.queueCount > index)
				{
					if (flags & VK_QUEUE_GRAPHICS_BIT) {
						m_GraphicsIndex = i;
						m_GraphicsQueue = queue.queue;
					}
					if (flags & VK_QUEUE_COMPUTE_BIT) {
						m_ComputeIndex = i;
						m_ComputeQueue = queue.queue;
					}
					if (surface != VK_NULL_HANDLE) {
						VkBool32 support = VK_FALSE;
						vkGetPhysicalDeviceSurfaceSupportKHR(m_PhysicalDevice, index, surface, &support);
						if (!support) {
							continue;
						}
					}
					return queue;
				}
			}

			DP_ERROR("Queue not found");
			throw std::runtime_error("Queue not found");
		}

		bool GraphicsDeviceVK::QueryPresentSupport(uint32_t index, VkSurfaceKHR surface)
		{
			VkBool32 support = VK_FALSE;
			vkGetPhysicalDeviceSurfaceSupportKHR(m_PhysicalDevice, index, surface, &support);
			return support;
		}

		void GraphicsDeviceVK::CreateInstance()
		{
			VkValidationFeatureEnableEXT enables[] = {
				VK_VALIDATION_FEATURE_ENABLE_BEST_PRACTICES_EXT,
				VK_VALIDATION_FEATURE_ENABLE_GPU_ASSISTED_EXT,
				VK_VALIDATION_FEATURE_ENABLE_GPU_ASSISTED_RESERVE_BINDING_SLOT_EXT
			};
			VkValidationFeaturesEXT features = {};
			features.sType = VK_STRUCTURE_TYPE_VALIDATION_FEATURES_EXT;
			if (enableValidation)
			{
				features.enabledValidationFeatureCount = 1;
				features.pEnabledValidationFeatures = enables;
			}
			m_EnabledExtensions = InitInstanceExtensions(std::vector<const char*>());
			VkInstanceCreateInfo instInfo = {};
			VkApplicationInfo appInfo = {};
			appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
			appInfo.apiVersion = VK_MAKE_VERSION(1, 2, 0);
			appInfo.applicationVersion = VK_MAKE_VERSION(0, 1, 0);
			appInfo.engineVersion = VK_MAKE_VERSION(0, 1, 0);
			appInfo.pApplicationName = "DrawingPad-Test";
			appInfo.pEngineName = "DrawingPad";

			instInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
			instInfo.pNext = &features;
			instInfo.pApplicationInfo = &appInfo;

			instInfo.enabledExtensionCount = static_cast<uint32_t>(m_EnabledExtensions.size());
			instInfo.ppEnabledExtensionNames = m_EnabledExtensions.data();

			VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo = {};
			if (enableValidation) {
				m_EnabledLayers = InitInstanceLayers(std::vector<const char*>());
				instInfo.enabledLayerCount = static_cast<uint32_t>(m_EnabledLayers.size());
				instInfo.ppEnabledLayerNames = m_EnabledLayers.data();

				debugCreateInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
				debugCreateInfo.flags = 0;
				debugCreateInfo.pNext = nullptr;
				debugCreateInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT;
				debugCreateInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
				debugCreateInfo.pfnUserCallback = debugCallback;
				debugCreateInfo.pUserData = nullptr;
			}
			else {
				instInfo.enabledLayerCount = 0;
				instInfo.ppEnabledLayerNames = nullptr;
			}

			if (vkCreateInstance(&instInfo, nullptr, &m_Instance) != VK_SUCCESS) {
				DP_CRITICAL("Failed to create VkInstance");
				throw std::runtime_error("Failed to create instance");
			}


			if (!enableValidation) return;
			CreateDebugUtilsMessengerEXT = reinterpret_cast<PFN_vkCreateDebugUtilsMessengerEXT>(vkGetInstanceProcAddr(m_Instance, "vkCreateDebugUtilsMessengerEXT"));
			DestroyDebugUtilsMessengerEXT = reinterpret_cast<PFN_vkDestroyDebugUtilsMessengerEXT>(vkGetInstanceProcAddr(m_Instance, "vkDestroyDebugUtilsMessengerEXT"));
			if (CreateDebugUtilsMessengerEXT(m_Instance, &debugCreateInfo, nullptr, &m_DebugMessenger) != VK_SUCCESS) {
				DP_ERROR("Failed to create debug messenger"); 
				throw std::runtime_error("failed to set up debug messenger");
			}
		}

		void GraphicsDeviceVK::CreatePhysicalDevice()
		{
			uint32_t deviceCount;
			vkEnumeratePhysicalDevices(m_Instance, &deviceCount, nullptr);
			if (deviceCount == 0) {
				DP_CRITICAL("Failed to find a GPU with Vulkan support");
				throw std::runtime_error("failed to find a GPU with Vulkan support");
			}

			std::vector<VkPhysicalDevice> devices(deviceCount);
			vkEnumeratePhysicalDevices(m_Instance, &deviceCount, devices.data());

			DP_INFO("========================================");
			DP_INFO("Enumerating Graphics Devices:");
			DP_INFO("========================================");
			for (const auto& device : devices)
			{
				auto props = VkPhysicalDeviceProperties{};
				vkGetPhysicalDeviceProperties(device, &props);
				UtilsVK::PrintDeviceProps(props);
				DP_INFO("========================================");
			}
			for (const auto& device : devices)
			{
				auto props = VkPhysicalDeviceProperties{};
				vkGetPhysicalDeviceProperties(device, &props);
				if (props.deviceType == VkPhysicalDeviceType::VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) {
					m_PhysicalDevice = device;
					break;
				}
				else if (props.deviceType == VkPhysicalDeviceType::VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU) {
					m_PhysicalDevice = device;
				}
			}

			m_PhysicalFeats12.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES;
			m_PhysicalFeats11.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_1_FEATURES;
			m_PhysicalFeats11.pNext = &m_PhysicalFeats12;
			m_PhysicalFeats2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
			m_PhysicalFeats2.pNext = &m_PhysicalFeats11;

			vkGetPhysicalDeviceFeatures(m_PhysicalDevice, &m_PhysicalFeats);
			vkGetPhysicalDeviceFeatures2(m_PhysicalDevice, &m_PhysicalFeats2);
			vkGetPhysicalDeviceProperties(m_PhysicalDevice, &m_PhysicalProps);
			vkGetPhysicalDeviceMemoryProperties(m_PhysicalDevice, &m_PhysicalMemProps);

			uint32_t queueCount = 0;
			vkGetPhysicalDeviceQueueFamilyProperties(m_PhysicalDevice, &queueCount, nullptr);
			m_QueueFamilyProps = std::vector<VkQueueFamilyProperties>(queueCount);
			vkGetPhysicalDeviceQueueFamilyProperties(m_PhysicalDevice, &queueCount, m_QueueFamilyProps.data());

			VkPhysicalDeviceProperties props;
			vkGetPhysicalDeviceProperties(m_PhysicalDevice, &props);
			DP_INFO("Chosen Graphics Device:");
			DP_INFO("========================================");
			UtilsVK::PrintDeviceProps(props);
			DP_INFO("========================================");

			m_PhysicalLimits = props.limits;

			VkSampleCountFlags counts = m_PhysicalLimits.framebufferColorSampleCounts & m_PhysicalLimits.framebufferDepthSampleCounts;
			if (counts & VK_SAMPLE_COUNT_64_BIT) m_SampleCount = VK_SAMPLE_COUNT_64_BIT;
			else if (counts & VK_SAMPLE_COUNT_32_BIT) m_SampleCount = VK_SAMPLE_COUNT_32_BIT;
			else if (counts & VK_SAMPLE_COUNT_16_BIT) m_SampleCount = VK_SAMPLE_COUNT_16_BIT;
			else if (counts & VK_SAMPLE_COUNT_8_BIT)  m_SampleCount = VK_SAMPLE_COUNT_8_BIT;
			else if (counts & VK_SAMPLE_COUNT_4_BIT)  m_SampleCount = VK_SAMPLE_COUNT_4_BIT;
			else if (counts & VK_SAMPLE_COUNT_2_BIT)  m_SampleCount = VK_SAMPLE_COUNT_2_BIT;
			else m_SampleCount = VK_SAMPLE_COUNT_1_BIT;
		}

		void GraphicsDeviceVK::CreateDevice()
		{
			// Create device queues
			std::vector<VkDeviceQueueCreateInfo> queueCreateInfos(m_QueueFamilyProps.size());
			std::vector<std::vector<float>> queuePriorities(m_QueueFamilyProps.size());

			for (auto i = 0; i < m_QueueFamilyProps.size(); i++) {
				queuePriorities[i].resize(m_QueueFamilyProps[i].queueCount, 0.5f);

				queueCreateInfos[i].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
				queueCreateInfos[i].queueFamilyIndex = i;
				queueCreateInfos[i].queueCount = m_QueueFamilyProps[i].queueCount;
				queueCreateInfos[i].pQueuePriorities = queuePriorities[i].data();
			}

			deviceExtensions = InitDeviceExtensions({ VK_KHR_SWAPCHAIN_EXTENSION_NAME });

			VkDeviceCreateInfo createInfo = {};
			createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
			createInfo.pNext = &m_PhysicalFeats2;
			createInfo.flags = 0;
			createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
			createInfo.pQueueCreateInfos = queueCreateInfos.data();
			//createInfo.pEnabledFeatures = &m_PhysicalFeats;
			createInfo.enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size());
			createInfo.ppEnabledExtensionNames = deviceExtensions.data();
			createInfo.enabledLayerCount = 0;
			createInfo.ppEnabledLayerNames = nullptr;
			if (vkCreateDevice(m_PhysicalDevice, &createInfo, nullptr, &m_Device) != VK_SUCCESS) {
				throw std::runtime_error("failed to create logical device");
			}
			m_Queues.resize(m_QueueFamilyProps.size());
			for (auto fi = 0; fi < m_QueueFamilyProps.size(); fi++) {
				VkBool32 present;
				vkGetPhysicalDeviceSurfaceSupportKHR(m_PhysicalDevice, fi, m_Surface, &present);
				for (uint32_t i = 0; i < m_QueueFamilyProps[fi].queueCount; i++) {
					Queue queue = {};
					queue.familyIndex = fi;
					queue.index = i;
					queue.presentSupport = present;
					queue.properties = m_QueueFamilyProps[fi];
					vkGetDeviceQueue(m_Device, fi, i, &queue.queue);

					m_Queues[fi].emplace_back(queue);
				}
			}

			VmaVulkanFunctions vmaFunc = {};
			vmaFunc.vkAllocateMemory = vkAllocateMemory;
			vmaFunc.vkBindBufferMemory = vkBindBufferMemory;
			vmaFunc.vkBindImageMemory = vkBindImageMemory;
			vmaFunc.vkCreateBuffer = vkCreateBuffer;
			vmaFunc.vkCreateImage = vkCreateImage;
			vmaFunc.vkDestroyBuffer = vkDestroyBuffer;
			vmaFunc.vkDestroyImage = vkDestroyImage;
			vmaFunc.vkFlushMappedMemoryRanges = vkFlushMappedMemoryRanges;
			vmaFunc.vkFreeMemory = vkFreeMemory;
			vmaFunc.vkGetBufferMemoryRequirements = vkGetBufferMemoryRequirements;
			vmaFunc.vkGetImageMemoryRequirements = vkGetImageMemoryRequirements;
			vmaFunc.vkGetPhysicalDeviceMemoryProperties = vkGetPhysicalDeviceMemoryProperties;
			vmaFunc.vkGetPhysicalDeviceProperties = vkGetPhysicalDeviceProperties;
			vmaFunc.vkInvalidateMappedMemoryRanges = vkInvalidateMappedMemoryRanges;
			vmaFunc.vkMapMemory = vkMapMemory;
			vmaFunc.vkUnmapMemory = vkUnmapMemory;
			vmaFunc.vkCmdCopyBuffer = vkCmdCopyBuffer;

			VmaAllocatorCreateInfo allocInfo = {};
			allocInfo.vulkanApiVersion = VK_API_VERSION_1_1;
			allocInfo.physicalDevice = m_PhysicalDevice;
			allocInfo.device = m_Device;
			allocInfo.instance = m_Instance;
			if (IsExtensionAvailable(VK_KHR_GET_MEMORY_REQUIREMENTS_2_EXTENSION_NAME) && IsExtensionAvailable(VK_KHR_DEDICATED_ALLOCATION_EXTENSION_NAME))
			{
				allocInfo.flags |= VMA_ALLOCATOR_CREATE_KHR_DEDICATED_ALLOCATION_BIT;
				vmaFunc.vkGetBufferMemoryRequirements2KHR = vkGetBufferMemoryRequirements2;
				vmaFunc.vkGetImageMemoryRequirements2KHR = vkGetImageMemoryRequirements2;
			}
			allocInfo.pVulkanFunctions = &vmaFunc;
			if (vmaCreateAllocator(&allocInfo, &m_MemAllocator) != VK_SUCCESS) {
				throw std::runtime_error("Failed to create VMA Allocator");
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

		std::vector<const char*> GraphicsDeviceVK::InitDeviceExtensions(std::vector<const char*> extns)
		{
			uint32_t extensionCount = 0;
			auto extensions = vkEnumerateDeviceExtensionProperties(m_PhysicalDevice, nullptr, &extensionCount, nullptr);
			std::vector<VkExtensionProperties> deviceExtProps(extensionCount);
			vkEnumerateDeviceExtensionProperties(m_PhysicalDevice, nullptr, &extensionCount, deviceExtProps.data());

			m_ExtensionProps.insert(m_ExtensionProps.end(), std::make_move_iterator(deviceExtProps.begin()), std::make_move_iterator(deviceExtProps.end()));

			if (IsExtensionAvailable(VK_KHR_PERFORMANCE_QUERY_EXTENSION_NAME) && IsExtensionAvailable(VK_EXT_HOST_QUERY_RESET_EXTENSION_NAME))
			{
				extns.emplace_back(VK_KHR_PERFORMANCE_QUERY_EXTENSION_NAME);
				extns.emplace_back(VK_EXT_HOST_QUERY_RESET_EXTENSION_NAME);
			}

			return extns;
		}

		std::vector<const char*> GraphicsDeviceVK::InitInstanceExtensions(std::vector<const char*> extns)
		{
			uint32_t extensionCount = 0;
			auto extensions = vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);
			m_ExtensionProps.resize(extensionCount);
			vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, m_ExtensionProps.data());

			uint32_t glfwExtensionCount = 0;
			const char** glfwExtensions;
			glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);
			for (uint32_t i = 0; i < glfwExtensionCount; i++) {
				if (!IsExtensionAvailable(glfwExtensions[i]))
				{
					DP_ERROR("Required extension not found!");
					throw std::runtime_error("Required extension not found!");
				}
				extns.emplace_back(glfwExtensions[i]);
			}

			if (enableValidation && IsExtensionAvailable(VK_EXT_VALIDATION_FEATURES_EXTENSION_NAME))
				extns.emplace_back(VK_EXT_VALIDATION_FEATURES_EXTENSION_NAME);

			if (IsExtensionAvailable(VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME))
				extns.emplace_back(VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME);

			if (enableValidation) {
				if (IsExtensionAvailable(VK_EXT_DEBUG_UTILS_EXTENSION_NAME))
					extns.emplace_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
				else
				{
					DP_CRITICAL("Extension VK_EXT_debug_utils not available");
					throw std::runtime_error("Extension VK_EXT_debug_utils not available");
				}
			}
			return extns;
		}

		std::vector<const char*> GraphicsDeviceVK::InitInstanceLayers(std::vector<const char*>& lyrs)
		{
			uint32_t layerCount = 0;
			auto extensions = vkEnumerateInstanceLayerProperties(&layerCount, nullptr);
			m_LayerProps.resize(layerCount);
			vkEnumerateInstanceLayerProperties(&layerCount, m_LayerProps.data());

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
						DP_CRITICAL("Vulkan validation layer not found");
						throw std::runtime_error("Vulkan validation layer not found");
					}
					lyrs.emplace_back(name);
				}
			}
			else {
				if (IsLayerAvailable("VK_LAYER_KHRONOS_validation"))
					lyrs.emplace_back("VK_LAYER_KHRONOS_validation");
				else {
					DP_CRITICAL("Layer VK_LAYER_KHRONOS_validation not found");
					throw std::runtime_error("Layer VK_LAYER_KHRONOS_validation not found");
				}
			}
			return lyrs;
		}
	}
}
