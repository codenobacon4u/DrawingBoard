#pragma once

#include "Swapchain.h"

#include "GraphicsContextVK.h"
#include "TextureVK.h"

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <Vulkan/vulkan.h>

namespace VkAPI
{
	struct SwapSupportDetails {
		VkSurfaceCapabilitiesKHR capabilities;
		std::vector<VkSurfaceFormatKHR> formats;
		std::vector<VkPresentModeKHR> presentModes;
	};

	struct SwapBuffer {
		VkImage image;
		VkImageView view;
	};

	class GraphicsDeviceVK;
	class SwapchainVK : public Swapchain
	{
	public:
		SwapchainVK(GraphicsDeviceVK* device, GraphicsContextVK* context, SwapchainDesc desc, GLFWwindow* window);

		~SwapchainVK();

		virtual void Resize(uint32_t width, uint32_t height) override;

		virtual void Present(uint32_t sync) override;
		bool AcquireNextImage();

		void* GetNative() { return &m_Swap; }
		VkQueue GetPresentQueue() { return m_Present; }
		virtual uint32_t GetImageIndex() override { return m_ImageIndex; }

		virtual TextureView* GetNextBackbuffer() override { return m_BackBuffers[m_ImageIndex].second; }
		virtual TextureViewVK* GetDepthBufferView() override { return m_DepthBuffer; }

		void SetResized(uint32_t width, uint32_t height) { m_Resized = true; m_Desc.Width = width; m_Desc.Height = height; }
	private:
		void RecreateSwap(uint32_t width, uint32_t height);
		void Cleanup();

		SwapSupportDetails QuerySwapSupport();
		VkSurfaceFormatKHR ChooseSwapSurfaceFormat(std::vector<TextureFormat>& requestedFormats, std::vector<VkSurfaceFormatKHR>& availableFormats);
		VkPresentModeKHR ChooseSwapPresentMode(std::vector<VkPresentModeKHR>& availablePresentModes);
		VkExtent2D ChooseSwapExtent(VkSurfaceCapabilitiesKHR& capabilities, uint32_t width, uint32_t height);
		TextureFormat ChooseDepthFormat();
		void CreatePresentResources(VkSurfaceFormatKHR surfaceFormat);
	private:
		GraphicsDeviceVK* m_Device;

		VkSurfaceKHR m_Surface;
		VkExtent2D m_Extent;
		std::vector<std::pair<TextureVK*,TextureViewVK*>> m_BackBuffers;
		TextureViewVK* m_DepthBuffer = VK_NULL_HANDLE;
		VkPresentModeKHR m_PresentMode;
		VkSwapchainKHR m_Swap;

		std::vector<VkSemaphore> m_ImageAcquiredSemaphores;
		std::vector<VkSemaphore> m_DrawCompleteSemaphores;
		std::vector<VkFence> m_FlightFences;
		std::vector<VkFence> m_ImagesInFlight;

		std::vector<bool> m_SwapImagesInitialized;
		std::vector<bool> m_ImageAcquiredFenceSubmitted;

		VkQueue m_Present;
		uint32_t m_PresentIndex;

		uint32_t m_ImageIndex = 0;
		uint32_t m_CurrFrame = 0;
		bool m_Resized = false;
		const uint32_t FRAMES_IN_FLIGHT = 3;
		GraphicsContextVK* m_Context;
	};
}