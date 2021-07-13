#include "pwpch.h"
#include "TextureVK.h"

#include "GraphicsDeviceVK.h"
#include "UtilsVK.h"

namespace VkAPI
{
	TextureVK::TextureVK(GraphicsDeviceVK* device, const TextureDesc& desc, const void* data)
		: Texture(desc), m_Device(device), m_Data(data)
	{
		VkImageCreateInfo imageInfo = {};
		imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
		if (m_Desc.Type == TextureType::DimTex1D || m_Desc.Type == TextureType::DimTex1DArray)
			imageInfo.imageType = VK_IMAGE_TYPE_1D;
		else if (m_Desc.Type == TextureType::DimTex2D || m_Desc.Type == TextureType::DimTex2DArray)
			imageInfo.imageType = VK_IMAGE_TYPE_2D;
		else if (m_Desc.Type == TextureType::DimTex3D)
			imageInfo.imageType = VK_IMAGE_TYPE_3D;
		imageInfo.extent.width = m_Desc.Width;
		imageInfo.extent.height = (m_Desc.Type == TextureType::DimTex1D || m_Desc.Type == TextureType::DimTex1DArray) ? 1 : m_Desc.Height;
		imageInfo.extent.depth = (m_Desc.Type == TextureType::DimTex3D) ? m_Desc.Depth : 1;
		imageInfo.mipLevels = m_Desc.MipLevels;
		if (m_Desc.Type == TextureType::DimTex1DArray || m_Desc.Type == TextureType::DimTex2DArray)
			imageInfo.arrayLayers = m_Desc.ArraySize;
		else
			imageInfo.arrayLayers = 1;
		imageInfo.format = UtilsVK::Convert(m_Desc.Format);
		imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
		imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		imageInfo.usage = 0;
		if ((uint32_t)m_Desc.BindFlags & (uint32_t)BindFlags::RenderTarget)
			imageInfo.usage |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
		if ((uint32_t)m_Desc.BindFlags & (uint32_t)BindFlags::DepthStencil)
			imageInfo.usage |= VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
		if ((uint32_t)m_Desc.BindFlags & (uint32_t)BindFlags::ShaderResource)
			imageInfo.usage |= VK_IMAGE_USAGE_SAMPLED_BIT;

		imageInfo.samples = static_cast<VkSampleCountFlagBits>(m_Desc.SampleCount);
		imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
		imageInfo.queueFamilyIndexCount = 0;
		imageInfo.pQueueFamilyIndices = nullptr;
		vkCreateImage(m_Device->Get(), &imageInfo, nullptr, &m_Image);

		VkMemoryRequirements memReq;
		vkGetImageMemoryRequirements(m_Device->Get(), m_Image, &memReq);

		VkMemoryAllocateInfo allocInfo = {};
		allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		allocInfo.allocationSize = memReq.size;
		allocInfo.memoryTypeIndex = m_Device->FindMemoryType(memReq.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

		vkAllocateMemory(m_Device->Get(), &allocInfo, nullptr, &m_Mem);

		vkBindImageMemory(m_Device->Get(), m_Image, m_Mem, 0);
	}

	TextureVK::TextureVK(GraphicsDeviceVK* device, const TextureDesc& desc, VkImage image)
		: Texture(desc), m_Device(device), m_Image(image), m_Data(nullptr), m_Mem(VK_NULL_HANDLE)
	{
	}

	TextureVK::~TextureVK()
	{
		if (m_Image)
			vkDestroyImage(m_Device->Get(), m_Image, nullptr);
		vkUnmapMemory(m_Device->Get(), m_Mem);
	}

	TextureView* TextureVK::CreateView(const TextureViewDesc& desc)
	{
		auto updatedDesc = desc;
		if (desc.NumMipLevels == 0)
			if (desc.ViewType == ViewType::ShaderResource)
				updatedDesc.NumMipLevels = m_Desc.MipLevels - desc.HighestMip;
			else
				updatedDesc.NumMipLevels = 1;

		if (desc.Slices == 0)
			if (desc.Dim == TextureType::DimTex1DArray || desc.Dim == TextureType::DimTex2DArray)
				updatedDesc.Slices = m_Desc.ArraySize - desc.FirstSlice;
			else if (desc.Dim == TextureType::DimTex3D)
				updatedDesc.Slices = (m_Desc.Depth >> desc.HighestMip) - desc.FirstSlice;
			else
				updatedDesc.Slices = 1;

		return new TextureViewVK(m_Device, updatedDesc, this, VK_IMAGE_ASPECT_COLOR_BIT);
	}

	TextureViewVK::TextureViewVK(GraphicsDeviceVK* device, const TextureViewDesc& desc, TextureVK* texture, VkImageAspectFlags aspectFlags)
		: TextureView(desc), m_Device(device), m_Texture(texture)
	{
		VkImageViewCreateInfo viewInfo = {};
		viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		viewInfo.image = m_Texture->GetImage();
		viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
		viewInfo.format = UtilsVK::Convert(desc.Format);
		viewInfo.components = {
			VK_COMPONENT_SWIZZLE_IDENTITY,
			VK_COMPONENT_SWIZZLE_IDENTITY,
			VK_COMPONENT_SWIZZLE_IDENTITY,
			VK_COMPONENT_SWIZZLE_IDENTITY
		};
		viewInfo.subresourceRange.baseMipLevel = desc.HighestMip;
		viewInfo.subresourceRange.levelCount = desc.NumMipLevels;
		if (desc.Dim == TextureType::DimTex1DArray || desc.Dim == TextureType::DimTex2DArray)
		{
			viewInfo.subresourceRange.baseArrayLayer = desc.FirstSlice;
			viewInfo.subresourceRange.layerCount = desc.Slices;
		}
		else
		{
			viewInfo.subresourceRange.baseArrayLayer = 0;
			viewInfo.subresourceRange.layerCount = 1;
		}
		viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;

		vkCreateImageView(device->Get(), &viewInfo, nullptr, &m_View);
	}

	TextureViewVK::~TextureViewVK()
	{
		if (m_Desc.ViewType == ViewType::DepthStencil || m_Desc.ViewType == ViewType::RenderTarget)
			m_Device->GetFramebufferPool().DeleteViewEntry(m_View);
		vkDestroyImageView(m_Device->Get(), m_View, nullptr);
	}
}