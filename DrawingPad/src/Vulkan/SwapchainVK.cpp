#include "dppch.h"
#include "SwapchainVK.h"

#include "GraphicsDeviceVK.h"
#include "TextureVK.h"
#include "DebugVK.h"

namespace Vulkan
{
	SwapchainVK::SwapchainVK(GraphicsDeviceVK* device, SwapchainDesc desc, VkSurfaceKHR surface)
		: Swapchain(desc), m_Device(device), m_Handle(nullptr), m_Surface(surface)
	{
		m_PresentQueue = m_Device->GetQueueByFlags(VK_QUEUE_GRAPHICS_BIT, 0, m_Surface);

		RecreateSwap(desc.Width, desc.Height);
	}

	SwapchainVK::~SwapchainVK()
	{
		vkDeviceWaitIdle(m_Device->Get());
		Cleanup();
		vkDestroySwapchainKHR(m_Device->Get(), m_Handle, nullptr);
		vkDestroySurfaceKHR(m_Device->GetInstance(), m_Surface, nullptr);
	}

	void SwapchainVK::Resize(uint32_t width, uint32_t height)
	{
		m_Device->WaitForIdle();
		RecreateSwap(width, height);
	}

	bool SwapchainVK::AcquireNextImage(VkSemaphore acquired)
	{
		// We need the imageAcquired semaphores to be sent out for the command buffer submission
		VkResult result = vkAcquireNextImageKHR(m_Device->Get(), m_Handle, ULONG_MAX, acquired, VK_NULL_HANDLE, &m_ImageIndex);
		if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR) {
			RecreateSwap(m_Desc.Width, m_Desc.Height);
			result = vkAcquireNextImageKHR(m_Device->Get(), m_Handle, ULONG_MAX, acquired, VK_NULL_HANDLE, &m_ImageIndex);
		}
		if (result != VK_SUCCESS)
			throw std::runtime_error("Could not get next image from the swapchain");
		return true;
	}

	void SwapchainVK::Present(VkQueue queue, VkSemaphore render)
	{
		VkPresentInfoKHR presentInfo = {};
		presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
		presentInfo.waitSemaphoreCount = 1;
		presentInfo.pWaitSemaphores = &render;
		presentInfo.swapchainCount = 1;
		presentInfo.pSwapchains = &m_Handle;
		presentInfo.pImageIndices = &m_ImageIndex;
		auto result = vkQueuePresentKHR(queue, &presentInfo);

		if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || m_Resized)
		{
			m_Resized = false;
			RecreateSwap(m_Desc.Width, m_Desc.Height);
		}
		else if (result != VK_SUCCESS)
			throw std::runtime_error("Failed to present swap image!");

		m_CurrFrame = (m_CurrFrame + 1) % m_BackBuffers.size();
	}

	void SwapchainVK::RecreateSwap(uint32_t width, uint32_t height)
	{
		m_Device->WaitForIdle();

		Cleanup();

		m_ImageIndex = 0;
		SwapSupportDetails support = QuerySwapSupport();
		auto surfaceFormat = ChooseSwapSurfaceFormat(m_Desc.SurfaceFormats, support.formats);
		m_PresentMode = ChooseSwapPresentMode(support.presentModes);
		m_Extent = ChooseSwapExtent(support.capabilities, width, height);

		uint32_t imageCount = support.capabilities.minImageCount + 1;
		if (support.capabilities.maxImageCount > 0 && imageCount > support.capabilities.maxImageCount) {
			imageCount = support.capabilities.maxImageCount;
		}

		m_Desc.Width = m_Extent.width;
		m_Desc.Height = m_Extent.height;
		if (m_Desc.DepthFormat != TextureFormat::None)
			m_Desc.DepthFormat = ChooseDepthFormat();

		auto old = m_Handle;
		m_Handle = nullptr;

		VkSwapchainCreateInfoKHR createInfo = {};
		createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
		createInfo.pNext = nullptr;
		createInfo.flags = 0;
		createInfo.surface = m_Surface;
		createInfo.minImageCount = imageCount;
		createInfo.imageFormat = surfaceFormat.format;
		createInfo.imageColorSpace = surfaceFormat.colorSpace;
		createInfo.imageExtent = m_Extent;
		createInfo.imageArrayLayers = 1;
		createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
		createInfo.queueFamilyIndexCount = 0;
		createInfo.pQueueFamilyIndices = NULL;
		createInfo.presentMode = m_PresentMode;
		createInfo.preTransform = support.capabilities.currentTransform;
		createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
		createInfo.clipped = true;
		createInfo.oldSwapchain = old;
		createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
		m_Desc.ColorFormat = UtilsVK::VkToTextureFormat(surfaceFormat.format);

		if (vkCreateSwapchainKHR(m_Device->Get(), &createInfo, nullptr, &m_Handle) != VK_SUCCESS) {
			throw std::runtime_error("Failed to create swapchain");
		}

		DebugMarker::SetName(m_Device->Get(), (uint64_t)m_Handle, VK_OBJECT_TYPE_SWAPCHAIN_KHR, "Main Swapchain");

		if (old != nullptr)
		{
			vkDestroySwapchainKHR(m_Device->Get(), old, NULL);
			old = nullptr;
		}


		uint32_t swapImageCount;
		vkGetSwapchainImagesKHR(m_Device->Get(), m_Handle, &swapImageCount, nullptr);
		if (swapImageCount != m_Desc.BufferCount)
			m_Desc.BufferCount = swapImageCount;

		CreatePresentResources(surfaceFormat);
	}

	void SwapchainVK::Cleanup()
	{
		if (m_Desc.DepthFormat != TextureFormat::None)
		{
			delete m_DepthTextureView;
			delete m_DepthTexture;
		}

		for (uint32_t i = 0; i < m_BackBuffers.size(); i++)
		{
			delete m_BackBuffers[i].second;
			delete m_BackBuffers[i].first;
		}
		m_BackBuffers.clear();
	}

	SwapSupportDetails SwapchainVK::QuerySwapSupport()
	{
		SwapSupportDetails details;

		vkGetPhysicalDeviceSurfaceCapabilitiesKHR(m_Device->GetPhysical(), m_Surface, &details.capabilities);

		uint32_t count;
		if ((vkGetPhysicalDeviceSurfacePresentModesKHR(m_Device->GetPhysical(), m_Surface, &count, nullptr) == VK_SUCCESS) && count) {
			details.presentModes.resize(count);
			if (vkGetPhysicalDeviceSurfacePresentModesKHR(m_Device->GetPhysical(), m_Surface, &count, reinterpret_cast<VkPresentModeKHR*>(details.presentModes.data())) != VK_SUCCESS)
				throw std::runtime_error("Failed to get present modes for surface");
		}

		count = 0;
		if ((vkGetPhysicalDeviceSurfaceFormatsKHR(m_Device->GetPhysical(), m_Surface, &count, nullptr) == VK_SUCCESS) && count) {
			details.formats.resize(count);
			if (vkGetPhysicalDeviceSurfaceFormatsKHR(m_Device->GetPhysical(), m_Surface, &count, reinterpret_cast<VkSurfaceFormatKHR*>(details.formats.data())) != VK_SUCCESS)
				throw std::runtime_error("Failed to get present modes for surface");
		}

		return details;
	}

	VkSurfaceFormatKHR SwapchainVK::ChooseSwapSurfaceFormat(std::vector<TextureFormat>& requestedFormats, std::vector<VkSurfaceFormatKHR>& availableFormats)
	{
		for (const auto& requestedFormat : requestedFormats) {
			for (const auto& availableFormat : availableFormats)
			{
				if (availableFormat.colorSpace == UtilsVK::TextureFormatToVk(requestedFormat) && availableFormat.format == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
					return availableFormat;
				}
			}
		}

		return availableFormats[0];
	}

	VkPresentModeKHR SwapchainVK::ChooseSwapPresentMode(std::vector<VkPresentModeKHR>& availablePresentModes)
	{
		for (const auto& availablePresentMode : availablePresentModes) {
			if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR) {
				return availablePresentMode;
			}
		}

		return VK_PRESENT_MODE_FIFO_KHR;
	}

	VkExtent2D SwapchainVK::ChooseSwapExtent(VkSurfaceCapabilitiesKHR& capabilities, uint32_t width, uint32_t height)
	{
		if (capabilities.currentExtent.width != UINT32_MAX) {
			return capabilities.currentExtent;
		}
		else {
			VkExtent2D actualExtent = { width, height };

			actualExtent.width = std::max(capabilities.minImageExtent.width, std::min(capabilities.maxImageExtent.width, actualExtent.width));
			actualExtent.height = std::max(capabilities.minImageExtent.height, std::min(capabilities.maxImageExtent.height, actualExtent.height));

			return actualExtent;
		}
	}

	TextureFormat SwapchainVK::ChooseDepthFormat()
	{
		
		std::vector<VkFormat> formats = { VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT };
		for (VkFormat format : formats)
		{
			VkFormatProperties props;
			vkGetPhysicalDeviceFormatProperties(m_Device->GetPhysical(), format, &props);

			if ((props.optimalTilingFeatures & VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT))
				return UtilsVK::VkToTextureFormat(format);
		}
		throw std::runtime_error("Fialed to find supported depth buffer format!");
	}

	void SwapchainVK::CreatePresentResources(VkSurfaceFormatKHR surfaceFormat)
	{
		uint32_t imageCount;
		vkGetSwapchainImagesKHR(m_Device->Get(), m_Handle, &imageCount, nullptr);
		m_BackBuffers.resize(imageCount);
		std::vector<VkImage> swapImages(imageCount);
		if (vkGetSwapchainImagesKHR(m_Device->Get(), m_Handle, &imageCount, swapImages.data()) != VK_SUCCESS)
			throw new std::runtime_error("Failed to get swapchain images");
		for (uint32_t i = 0; i < imageCount; i++)
		{	
			TextureDesc backDesc;
			backDesc.Type = TextureType::DimTex2D;
			backDesc.Width = m_Desc.Width;
			backDesc.Height = m_Desc.Height;
			backDesc.Format = m_Desc.ColorFormat;
			backDesc.BindFlags = BindFlags::SwapChain;
			backDesc.MipLevels = 1;
			m_BackBuffers[i].first = static_cast<TextureVK*>(m_Device->CreateTextureFromImage(backDesc, swapImages[i]));
			
			TextureViewDesc tvDesc;
			tvDesc.ViewType = ViewType::RenderTarget;
			tvDesc.Format = backDesc.Format;
			m_BackBuffers[i].second = static_cast<TextureViewVK*>(m_BackBuffers[i].first->CreateView(tvDesc));

			DebugMarker::SetName(m_Device->Get(), (uint64_t)m_BackBuffers[i].first->Get(), VK_OBJECT_TYPE_IMAGE, string_format("Backbuffer Image {}", i));
			DebugMarker::SetName(m_Device->Get(), (uint64_t)m_BackBuffers[i].second->Get(), VK_OBJECT_TYPE_IMAGE_VIEW, string_format("Backbuffer Image View {}", i));
		}

		if (m_Desc.DepthFormat != TextureFormat::None)
		{
			TextureDesc depthDesc = {};
			depthDesc.Type = TextureType::DimTex2D;
			depthDesc.Width = m_Desc.Width;
			depthDesc.Height = m_Desc.Height;
			depthDesc.Format = m_Desc.DepthFormat;
			depthDesc.SampleCount = 1;
			depthDesc.MipLevels = 1;
			depthDesc.Usage = Usage::Default;
			depthDesc.BindFlags = BindFlags::DepthStencil | BindFlags::SwapChain;
			m_DepthTexture = static_cast<TextureVK*>(m_Device->CreateTexture(depthDesc, nullptr));

			TextureViewDesc tvDesc;
			tvDesc.ViewType = ViewType::DepthStencil;
			tvDesc.Format = depthDesc.Format;
			m_DepthTextureView = static_cast<TextureViewVK*>(m_DepthTexture->CreateView(tvDesc));

			DebugMarker::SetName(m_Device->Get(), (uint64_t)m_DepthTexture->Get(), VK_OBJECT_TYPE_IMAGE, "DepthStencil Image");
			DebugMarker::SetName(m_Device->Get(), (uint64_t)m_DepthTextureView->Get(), VK_OBJECT_TYPE_IMAGE_VIEW, "DepthStencil Image View");
		}
	}
}
