#include "dppch.h"
#include "TextureVK.h"

#include "GraphicsDeviceVK.h"
#include "UtilsVK.h"
#include "BufferVK.h"

namespace DrawingPad
{
	namespace Vulkan
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
			imageInfo.format = UtilsVK::TextureFormatToVk(m_Desc.Format);
			imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
			imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
			imageInfo.usage = 0;
			if (m_Desc.BindFlags == BindFlags::RenderTarget || m_Desc.BindFlags == BindFlags::SwapChain)
				imageInfo.usage |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
			if (m_Desc.BindFlags == BindFlags::RenderTarget)
				imageInfo.usage |= VK_IMAGE_USAGE_SAMPLED_BIT;
			if ((uint32_t)m_Desc.BindFlags & (uint32_t)BindFlags::DepthStencil)
				imageInfo.usage |= VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
			if (m_Desc.BindFlags == BindFlags::ShaderResource)
				imageInfo.usage |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;

			imageInfo.samples = static_cast<VkSampleCountFlagBits>(m_Desc.SampleCount);
			imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
			imageInfo.queueFamilyIndexCount = 0;
			imageInfo.pQueueFamilyIndices = nullptr;


			VmaAllocationCreateInfo memInfo = {};
			memInfo.usage = VMA_MEMORY_USAGE_AUTO;

			vmaCreateImage(m_Device->GetMemoryAllocator(), &imageInfo, &memInfo, &m_Handle, &m_Alloc, nullptr);

			if (m_Desc.BindFlags != (BindFlags::SwapChain | BindFlags::DepthStencil)) {
				if (data != nullptr) {
					BufferDesc bufDesc = {};
					bufDesc.Usage = BufferUsageFlags::Staging;
					bufDesc.BindFlags = BufferBindFlags::Staging;
					bufDesc.Size = m_Desc.Width * m_Desc.Height * 4;
					BufferVK staging(m_Device, bufDesc);

					staging.Update(0, bufDesc.Size, data);

					VkQueue graphics = m_Device->GetGraphicsQueue();
					VkCommandBuffer cmd = static_cast<CommandBufferVK*>(m_Device->GetTempCommandPool().RequestCommandBuffer())->Get();
					VkCommandBufferBeginInfo beginInfo = {};
					beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
					beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
					vkBeginCommandBuffer(cmd, &beginInfo);

					TransistionLayout(cmd, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
					CopyFromBuffer(cmd, staging.Get(), m_Desc.Width, m_Desc.Height);
					if (m_Desc.MipLevels > 1)
						GenerateMipmaps(cmd);
					else
						TransistionLayout(cmd, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

					vkEndCommandBuffer(cmd);
					VkSubmitInfo submit = {};
					submit.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
					submit.commandBufferCount = 1;
					submit.pCommandBuffers = &cmd;
					vkQueueSubmit(graphics, 1, &submit, VK_NULL_HANDLE);
					vkQueueWaitIdle(graphics);
				}

				TextureViewDesc texDesc = {};
				texDesc.Format = m_Desc.Format;
				if ((uint32_t)m_Desc.BindFlags & (uint32_t)BindFlags::DepthStencil)
					texDesc.ViewType = ViewType::DepthStencil;
				else
					texDesc.ViewType = ViewType::ShaderResource;
				texDesc.HighestMip = 0;
				texDesc.NumMipLevels = m_Desc.MipLevels;

				m_DefaultView = CreateView(texDesc);

				VkSamplerCreateInfo sampler = {};
				sampler.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
				sampler.magFilter = VK_FILTER_LINEAR;
				sampler.minFilter = VK_FILTER_LINEAR;
				sampler.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
				sampler.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
				sampler.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
				sampler.anisotropyEnable = VK_TRUE;
				sampler.maxAnisotropy = m_Device->GetPhysicalLimits().maxSamplerAnisotropy;
				sampler.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
				sampler.unnormalizedCoordinates = VK_FALSE;
				sampler.compareEnable = VK_FALSE;
				sampler.compareOp = VK_COMPARE_OP_ALWAYS;
				sampler.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
				sampler.minLod = 0.0f;
				sampler.maxLod = static_cast<float>(m_Desc.MipLevels);
				sampler.mipLodBias = 0.0f;

				vkCreateSampler(m_Device->Get(), &sampler, nullptr, &m_Sampler);
			}
		}

		TextureVK::TextureVK(GraphicsDeviceVK* device, const TextureDesc& desc, VkImage image)
			: Texture(desc), m_Device(device), m_Handle(image), m_Data(nullptr), m_Mem(VK_NULL_HANDLE)
		{
		}

		TextureVK::~TextureVK()
		{
			if (m_Sampler != VK_NULL_HANDLE)
				vkDestroySampler(m_Device->Get(), m_Sampler, nullptr);
			if (m_DefaultView != nullptr)
				delete m_DefaultView;
			if (m_Desc.BindFlags != BindFlags::SwapChain)
				vmaDestroyImage(m_Device->GetMemoryAllocator(), m_Handle, m_Alloc);
			if (m_Mem != VK_NULL_HANDLE)
				vkFreeMemory(m_Device->Get(), m_Mem, nullptr);


		}

		TextureView* TextureVK::CreateView(const TextureViewDesc& desc)
		{
			TextureViewDesc updatedDesc = desc;
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

		void TextureVK::TransistionLayout(VkCommandBuffer cmd, VkImageLayout oldLayout, VkImageLayout newLayout)
		{
			VkImageMemoryBarrier barrier = {};
			barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
			barrier.oldLayout = oldLayout;
			barrier.newLayout = newLayout;
			barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			barrier.image = m_Handle;
			barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			barrier.subresourceRange.baseMipLevel = 0;
			barrier.subresourceRange.levelCount = m_Desc.MipLevels;
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
		}

		void TextureVK::CopyFromBuffer(VkCommandBuffer cmd, VkBuffer buffer, uint32_t width, uint32_t height)
		{
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

			vkCmdCopyBufferToImage(cmd, buffer, m_Handle, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);
		}

		void TextureVK::GenerateMipmaps(VkCommandBuffer cmd)
		{
			VkFormatProperties formatProps;
			vkGetPhysicalDeviceFormatProperties(m_Device->GetPhysical(), UtilsVK::TextureFormatToVk(m_Desc.Format), &formatProps);

			if (!(formatProps.optimalTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT))
				throw std::runtime_error("Texture Image Format does not support linear blitting!");

			int32_t mipWidth = static_cast<int32_t>(m_Desc.Width);
			int32_t mipHeight = static_cast<int32_t>(m_Desc.Height);

			VkImageMemoryBarrier barrier = {};
			barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
			barrier.image = m_Handle;
			barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			barrier.subresourceRange.baseArrayLayer = 0;
			barrier.subresourceRange.layerCount = 1;
			barrier.subresourceRange.levelCount = 1;

			for (uint32_t i = 1; i < m_Desc.MipLevels; i++) {
				barrier.subresourceRange.baseMipLevel = i - 1;
				barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
				barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
				barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
				barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;

				vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 1, &barrier);

				VkImageBlit blit{};
				blit.srcOffsets[0] = { 0, 0, 0 };
				blit.srcOffsets[1] = { mipWidth, mipHeight, 1 };
				blit.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
				blit.srcSubresource.mipLevel = i - 1;
				blit.srcSubresource.baseArrayLayer = 0;
				blit.srcSubresource.layerCount = 1;
				blit.dstOffsets[0] = { 0, 0, 0 };
				blit.dstOffsets[1] = { mipWidth > 1 ? mipWidth / 2 : 1, mipHeight > 1 ? mipHeight / 2 : 1, 1 };
				blit.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
				blit.dstSubresource.mipLevel = i;
				blit.dstSubresource.baseArrayLayer = 0;
				blit.dstSubresource.layerCount = 1;

				vkCmdBlitImage(cmd, m_Handle, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, m_Handle, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &blit, VK_FILTER_LINEAR);

				barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
				barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
				barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
				barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

				vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, nullptr, 0, nullptr, 1, &barrier);

				if (mipWidth > 1) mipWidth /= 2;
				if (mipHeight > 1) mipHeight /= 2;
			}

			barrier.subresourceRange.baseMipLevel = m_Desc.MipLevels - 1;
			barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
			barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
			barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

			vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, nullptr, 0, nullptr, 1, &barrier);
		}

		TextureViewVK::TextureViewVK(GraphicsDeviceVK* device, const TextureViewDesc& desc, TextureVK* texture, VkImageAspectFlags aspectFlags)
			: TextureView(desc), m_Device(device), m_Texture(texture)
		{
			VkImageViewCreateInfo viewInfo = {};
			viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
			viewInfo.image = m_Texture->Get();
			viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
			viewInfo.format = UtilsVK::TextureFormatToVk(desc.Format);
			if (desc.ViewType == ViewType::DepthStencil)
				viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
			else
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

			vkCreateImageView(device->Get(), &viewInfo, nullptr, &m_Handle);
		}

		TextureViewVK::~TextureViewVK()
		{
			vkDestroyImageView(m_Device->Get(), m_Handle, nullptr);
		}
	}
}
