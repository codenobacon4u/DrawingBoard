#pragma once
#include "DrawingPad/GraphicsDevice.h"

#include <optional>
#include <vector>
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <vulkan/vulkan.h>
#include <vma_mem_alloc.h>

#include "CommandPoolVK.h"
#include "DescriptorSetVK.h"
#include "FramebufferPoolVK.h"
#include "StructsVK.h"
#include "TextureVK.h"
#include "UtilsVK.h"

namespace DrawingPad
{
	namespace Vulkan
	{
		class GraphicsDeviceVK : public GraphicsDevice
		{
		public:
			GraphicsDeviceVK(GLFWwindow* window);
			~GraphicsDeviceVK();

			void SubmitCommandBuffer(const VkSubmitInfo& info, VkFence* fences);

			virtual void WaitForIdle() override;

			virtual GraphicsContext* CreateGraphicsContext(Swapchain* swap) override;
			virtual Buffer* CreateBuffer(const BufferDesc& desc, void* data) override;
			virtual Texture* CreateTexture(const TextureDesc& desc, const unsigned char* data) override;
			virtual RenderPass* CreateRenderPass(const RenderPassDesc& desc) override;
			virtual Pipeline* CreateGraphicsPipeline(const GraphicsPipelineDesc& desc, RenderPass* renderpass) override;
			virtual Pipeline* CreateComputePipeline(const ComputePipelineDesc& desc) override;
			virtual Swapchain* CreateSwapchain(const SwapchainDesc& desc, GLFWwindow* window) override;
			virtual Shader* CreateShader(const ShaderDesc& desc) override;
			virtual ShaderProgram* CreateShaderProgram(std::vector<Shader*> shaders) override;

			TextureVK* CreateTextureFromImage(const TextureDesc& desc, VkImage img);

			uint32_t FindMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties);

			VkDevice Get() { return m_Device; }
			VkPhysicalDevice GetPhysical() { return m_PhysicalDevice; }
			VkInstance GetInstance() { return m_Instance; }

			VmaAllocator& GetMemoryAllocator() { return m_MemAllocator; }

			CommandPoolVK& GetTempCommandPool() { return *m_TempPool; }
			FramebufferPoolVK& GetFramebufferPool() { return *m_FramebufferPool; }

			VkPhysicalDeviceLimits GetPhysicalLimits() { return m_PhysicalLimits; }
			VkPhysicalDeviceProperties GetPhysicalProperties() { return m_PhysicalProps; }

			VkSampleCountFlags GetMaxMSAA() { return m_SampleCount; }

			const uint32_t GetGraphicsIndex();
			const VkQueue GetGraphicsQueue();
			const Queue GetQueueByFlags(VkQueueFlags flags, uint32_t index, VkSurfaceKHR surface = VK_NULL_HANDLE);

			bool QueryPresentSupport(uint32_t index, VkSurfaceKHR surface);
		private:
			void CreateInstance();
			void CreatePhysicalDevice();
			void CreateDevice();

			bool IsExtensionAvailable(const char* extName) const;
			bool IsLayerAvailable(const char* lyrName) const;

			std::vector<const char*> InitInstanceExtensions(std::vector<const char*> extns);
			std::vector<const char*> InitInstanceLayers(std::vector<const char*>& lyrs);

			std::vector<const char*> InitDeviceExtensions(std::vector<const char*> extns);
		private:
			VkInstance m_Instance;
			VkDebugUtilsMessengerEXT m_DebugMessenger;
			VkSurfaceKHR m_Surface;
			VkPhysicalDevice m_PhysicalDevice;
			VkDevice m_Device;

			uint32_t m_GraphicsIndex = ~0U;
			uint32_t m_ComputeIndex = ~0U;
			VkQueue m_GraphicsQueue = VK_NULL_HANDLE;
			VkQueue m_ComputeQueue = VK_NULL_HANDLE;
			std::vector<std::vector<Queue>> m_Queues = {};

			CommandPoolVK* m_TempPool;
			FramebufferPoolVK* m_FramebufferPool;

			VkPhysicalDeviceLimits m_PhysicalLimits;
			VkPhysicalDeviceFeatures m_PhysicalFeats;
			VkPhysicalDeviceFeatures2 m_PhysicalFeats2 = {};
			VkPhysicalDeviceVulkan11Features m_PhysicalFeats11 = {};
			VkPhysicalDeviceVulkan12Features m_PhysicalFeats12 = {};
			VkPhysicalDeviceProperties m_PhysicalProps;
			VkPhysicalDeviceMemoryProperties m_PhysicalMemProps;

			VmaAllocator m_MemAllocator;

			std::vector<VkQueueFamilyProperties> m_QueueFamilyProps;
			std::vector<VkExtensionProperties> m_DeviceExtensionProps;

			VkSampleCountFlags m_SampleCount;

			std::vector<VkExtensionProperties> m_ExtensionProps;
			std::vector<VkLayerProperties> m_LayerProps;
			std::vector<const char*> m_EnabledExtensions;
			std::vector<const char*> m_EnabledLayers;

			GLFWwindow* m_MainWindow;
		};
	}
}
