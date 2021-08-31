#include "pwpch.h"
#include "TextureVK.h"

#include "GraphicsDeviceVK.h"
#include "UtilsVK.h"
#include "BufferVK.h"

namespace VkAPI
{
	TextureVK::TextureVK(GraphicsDeviceVK* device, const TextureDesc& desc, const unsigned char* data)
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
		if (m_Desc.BindFlags == BindFlags::RenderTarget)
			imageInfo.usage |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
		if (m_Desc.BindFlags == BindFlags::DepthStencil)
			imageInfo.usage |= VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
		if (m_Desc.BindFlags == BindFlags::ShaderResource)
			imageInfo.usage |= VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;

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

		{
			BufferDesc bufDesc = {};
			bufDesc.Usage = BufferUsageFlags::Staging;
			bufDesc.BindFlags = BufferBindFlags::Staging;
			bufDesc.Size = m_Desc.Width * m_Desc.Height * 4;
			BufferVK staging(m_Device, bufDesc, data);

			void* temp;
			vkMapMemory(m_Device->Get(), staging.GetMemory(), 0, bufDesc.Size, 0, &temp);
			memcpy(temp, data, static_cast<size_t>(bufDesc.Size));
			vkUnmapMemory(m_Device->Get(), staging.GetMemory());

			TransistionLayout(VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
			CopyFromBuffer(staging.Get(), m_Desc.Width, m_Desc.Height);
			TransistionLayout(VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
		}

		TextureViewDesc texDesc = {};
		texDesc.Format = m_Desc.Format;
		texDesc.ViewType = ViewType::ShaderResource;

		m_DefaultView = CreateView(texDesc);

		VkSamplerCreateInfo sampler = {};
		sampler.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
		sampler.magFilter = VK_FILTER_LINEAR;
		sampler.minFilter = VK_FILTER_LINEAR;
		sampler.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
		sampler.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
		sampler.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
		sampler.anisotropyEnable = VK_FALSE;
		sampler.maxAnisotropy = 1.0f;//m_Device->GetPhysicalLimits().maxSamplerAnisotropy;
		sampler.borderColor = VK_BORDER_COLOR_INT_OPAQUE_WHITE;
		sampler.unnormalizedCoordinates = VK_FALSE;
		sampler.compareEnable = VK_FALSE;
		sampler.compareOp = VK_COMPARE_OP_ALWAYS;
		sampler.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
		sampler.mipLodBias = 0.0f;
		sampler.minLod = 0.0f;
		sampler.maxLod = 0.0f;

		vkCreateSampler(m_Device->Get(), &sampler, nullptr, &m_Sampler);
	}

	TextureVK::TextureVK(GraphicsDeviceVK* device, const TextureDesc& desc, VkImage image)
		: Texture(desc), m_Device(device), m_Image(image), m_Data(nullptr), m_Mem(VK_NULL_HANDLE)
	{
	}

	TextureVK::~TextureVK()
	{
		if (!((uint32_t)m_Desc.BindFlags & (uint32_t)BindFlags::RenderTarget))
		{
			vkUnmapMemory(m_Device->Get(), m_Mem);
			if (m_Image)
				vkDestroyImage(m_Device->Get(), m_Image, nullptr);
			if (m_DefaultView != nullptr)
				delete m_DefaultView;
			vkDestroySampler(m_Device->Get(), m_Sampler, nullptr);
		}
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

		return DBG_NEW TextureViewVK(m_Device, updatedDesc, this, VK_IMAGE_ASPECT_COLOR_BIT);
	}

	void TextureVK::TransistionLayout(VkImageLayout oldLayout, VkImageLayout newLayout)
	{
		VkQueue graphics = m_Device->GetGraphicsQueue();
		VkCommandBuffer cmd = m_Device->GetTempCommandPool().GetBuffer();

		VkImageMemoryBarrier barrier = {};
		barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		barrier.oldLayout = oldLayout;
		barrier.newLayout = newLayout;
		barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barrier.image = m_Image;
		barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		barrier.subresourceRange.baseMipLevel = 0;
		barrier.subresourceRange.levelCount = 1;
		barrier.subresourceRange.baseArrayLayer = 0;
		barrier.subresourceRange.layerCount = 1;

		VkPipelineStageFlags srcStage;
		VkPipelineStageFlags dstStage;

		if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
			barrier.srcAccessMask = 0;
			barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
			srcStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
			dstStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
		}
		else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
			barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
			barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
			srcStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
			dstStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
		}
		else
			throw std::invalid_argument("Unsupported layout transition!");

		vkCmdPipelineBarrier(cmd, srcStage, dstStage, 0, 0, nullptr, 0, nullptr, 1, &barrier);

		vkEndCommandBuffer(cmd);
		VkSubmitInfo submit = {};
		submit.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		submit.commandBufferCount = 1;
		submit.pCommandBuffers = &cmd;
		vkQueueSubmit(graphics, 1, &submit, VK_NULL_HANDLE);
		vkQueueWaitIdle(graphics);
		//m_Device->GetTempCommandPool().ReturnBuffer(std::move(cmd));
	}

	void TextureVK::CopyFromBuffer(VkBuffer buffer, uint32_t width, uint32_t height)
	{
		VkQueue graphics = m_Device->GetGraphicsQueue();
		VkCommandBuffer cmd = m_Device->GetTempCommandPool().GetBuffer();

		VkBufferImageCopy region = {};
		region.bufferOffset = 0;
		region.bufferRowLength = 0;
		region.bufferImageHeight = 0;
		region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		region.imageSubresource.mipLevel = 0;
		region.imageSubresource.baseArrayLayer = 0;
		region.imageSubresource.layerCount = 1;
		region.imageOffset = { 0, 0, 0 };
		region.imageExtent = {
			width,
			height,
			1
		};

		vkCmdCopyBufferToImage(cmd, buffer, m_Image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

		vkEndCommandBuffer(cmd);
		VkSubmitInfo submit = {};
		submit.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		submit.commandBufferCount = 1;
		submit.pCommandBuffers = &cmd;
		vkQueueSubmit(graphics, 1, &submit, VK_NULL_HANDLE);
		vkQueueWaitIdle(graphics);
		//m_Device->GetTempCommandPool().ReturnBuffer(std::move(cmd));
	}

	TextureViewVK::TextureViewVK(GraphicsDeviceVK* device, const TextureViewDesc& desc, TextureVK* texture, VkImageAspectFlags aspectFlags)
		: TextureView(desc), m_Device(device), m_Texture(texture)
	{
		VkImageViewCreateInfo viewInfo = {};
		viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		viewInfo.image = m_Texture->GetImage();
		viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
		viewInfo.format = UtilsVK::Convert(desc.Format);
		viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
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

		vkCreateImageView(device->Get(), &viewInfo, nullptr, &m_View);
	}

	TextureViewVK::~TextureViewVK()
	{
		if (m_Desc.ViewType == ViewType::DepthStencil || m_Desc.ViewType == ViewType::RenderTarget)
			m_Device->GetFramebufferPool().DeleteViewEntry(m_View);
		vkDestroyImageView(m_Device->Get(), m_View, nullptr);
	}
}