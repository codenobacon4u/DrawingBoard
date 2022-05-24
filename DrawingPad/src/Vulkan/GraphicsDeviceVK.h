#pragma once

#include "GraphicsDevice.h"

#include <vulkan/vulkan.h>
#include <optional>
#include <vector>
#include <queue>

#include "TextureVK.h"
#include "DescriptorSetVK.h"
#include "UtilsVK.h"
#include "CommandPoolVK.h"

namespace Vulkan
{
	struct QueueFamilyIndices {
		// optional: may or may not have a value stored
		std::optional<uint32_t> graphicsFamily; // Graphics Queue Family
		std::optional<uint32_t> computeFamily; // Presentation Queue Family
		// Does this family have both graphics and surface support?
		bool isComplete() {
			return graphicsFamily.has_value() && computeFamily.has_value();
		}
	};

	class GraphicsDeviceVK : public GraphicsDevice
	{
	public:
		GraphicsDeviceVK();
		~GraphicsDeviceVK();

		void SubmitCommandBuffer(const VkSubmitInfo& info, VkFence* fences);

		virtual void WaitForIdle() override;
		virtual void Present() override;

		virtual Buffer* CreateBuffer(const BufferDesc& desc, void* data) override;
		virtual Texture* CreateTexture(const TextureDesc& desc, const unsigned char* data) override;
		virtual RenderPass* CreateRenderPass(const RenderPassDesc& desc) override;
		virtual Framebuffer* CreateFramebuffer(const FramebufferDesc& desc) override;
		virtual Pipeline* CreateGraphicsPipeline(const GraphicsPipelineDesc& desc, RenderPass* renderpass) override;
		virtual Pipeline* CreateComputePipeline(const ComputePipelineDesc& desc) override;
		virtual Swapchain* CreateSwapchain(const SwapchainDesc& desc, GLFWwindow* window) override;
		virtual Shader* CreateShader(const ShaderDesc& desc) override;

		TextureVK* CreateTextureFromImage(const TextureDesc& desc, VkImage img);

		uint32_t FindMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties);

		VkDevice Get() { return m_Device; }
		VkPhysicalDevice GetPhysical() { return m_PhysicalDevice; }
		VkInstance GetInstance() { return m_Instance; }

		uint32_t GetGraphicsIndex() { return m_GraphicsIndex; }
		VkQueue GetGraphicsQueue() { return m_GraphicsQueue; }

		CommandPoolVK& GetTempCommandPool() { return *m_TempPool; }

		VkPhysicalDeviceLimits GetPhysicalLimits() { return m_Limits; }
		VkPhysicalDeviceProperties GetPhysicalProperties() { return m_Props; }

		QueueFamilyIndices FindQueueFamilies(VkQueueFlags flags);
		bool QueryPresentSupport(uint32_t index, VkSurfaceKHR surface);
	private:
		void CreateInstance();
		void CreatePhysicalDevice();
		void CreateDevice();

		bool IsExtensionAvailable(const char* extName) const;
		bool IsLayerAvailable(const char* lyrName) const;

		std::vector<const char*> InitInstanceExtensions(std::vector<const char*> extns);
		std::vector<const char*> InitInstanceLayers(std::vector<const char*> lyrs);
	protected:
		virtual void SwapBuffers(Swapchain* swapchain) const override;
	private:
		VkInstance m_Instance;
		VkDebugUtilsMessengerEXT m_DebugMessenger;
		VkSurfaceKHR m_Surface;
		VkPhysicalDevice m_PhysicalDevice;
		VkDevice m_Device;

		uint32_t m_GraphicsIndex;
		uint32_t m_ComputeIndex;
		VkQueue m_GraphicsQueue;

		CommandPoolVK* m_TempPool;

		VkPhysicalDeviceLimits m_Limits;
		VkPhysicalDeviceProperties m_Props;

		std::vector<VkExtensionProperties> m_ExtensionProps;
		std::vector<VkLayerProperties> m_LayerProps;
		std::vector<const char*> m_EnabledExtensions = InitInstanceExtensions(std::vector<const char*>());
		std::vector<const char*> m_EnabledLayers = InitInstanceLayers(std::vector<const char*>());
	};
}
