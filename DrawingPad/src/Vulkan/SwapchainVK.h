#pragma once
#include "Swapchain.h"

#include <vulkan/vulkan.h>

#include "TextureVK.h"

namespace Vulkan
{
	struct SwapSupportDetails {
		VkSurfaceCapabilitiesKHR capabilities = {};
		std::vector<VkSurfaceFormatKHR> formats = {};
		std::vector<VkPresentModeKHR> presentModes = {};
	};

	class GraphicsDeviceVK;
	class SwapchainVK : public Swapchain
	{
	public:
		SwapchainVK(GraphicsDeviceVK* device, SwapchainDesc desc, VkSurfaceKHR surface);

		~SwapchainVK();

		virtual void Resize(uint32_t width, uint32_t height) override;

		virtual uint32_t GetImageIndex() override { return m_ImageIndex; }
		virtual TextureView* GetBackbuffer() override { return m_BackBuffers[m_ImageIndex].second; }
		virtual TextureViewVK* GetDepthBufferView() override { return m_DepthTextureView; }

		VkQueue GetPresentQueue() { return m_Present; }
		size_t GetBackbufferCount() { return m_BackBuffers.size(); }

		bool AcquireNextImage(VkSemaphore acquired);
		void Present(VkQueue queue, VkSemaphore render);
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
		TextureViewVK* m_DepthTextureView = nullptr;
		TextureVK* m_DepthTexture = nullptr;
		VkPresentModeKHR m_PresentMode;
		VkSwapchainKHR m_Handle;

		VkQueue m_Present;
		uint32_t m_PresentIndex;

		uint32_t m_ImageIndex = 0;
		uint32_t m_CurrFrame = 0;
	};
}
