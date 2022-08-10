#pragma once
#include "DrawingPad/Texture.h"

#include <vulkan/vulkan.h>
#include <vma_mem_alloc.h>

namespace DrawingPad
{
	namespace Vulkan
	{
		class GraphicsDeviceVK;
		class TextureViewVK;
		class TextureVK : public Texture
		{
		public:
			TextureVK(GraphicsDeviceVK* device, const TextureDesc& desc, const unsigned char* data = nullptr);
			TextureVK(GraphicsDeviceVK* device, const TextureDesc& desc, VkImage image);

			~TextureVK();

			VkImage Get() { return m_Handle; }
			VkSampler GetSampler() { return m_Sampler; }

			virtual TextureView* CreateView(const TextureViewDesc& desc) override;

		private:
			void TransistionLayout(VkCommandBuffer cmd, VkImageLayout oldLayout, VkImageLayout newLayout);
			void CopyFromBuffer(VkCommandBuffer cmd, VkBuffer buffer, uint32_t width, uint32_t height);
			void GenerateMipmaps(VkCommandBuffer cmd);
		private:
			GraphicsDeviceVK* m_Device = nullptr;
			VkImage m_Handle = VK_NULL_HANDLE;
			VmaAllocation m_Alloc = VK_NULL_HANDLE;
			VkDeviceMemory m_Mem = VK_NULL_HANDLE;
			VkSampler m_Sampler = VK_NULL_HANDLE;
			const void* m_Data = nullptr;
		};

		class TextureViewVK : public TextureView
		{
		public:
			TextureViewVK(GraphicsDeviceVK* device, const TextureViewDesc& desc, TextureVK* texture, VkImageAspectFlags aspectFlags);

			~TextureViewVK();

			VkImageView Get() { return m_Handle; }

			virtual TextureVK* GetTexture() override { return m_Texture; }

		private:
			GraphicsDeviceVK* m_Device = nullptr;
			VkImageView m_Handle = VK_NULL_HANDLE;
			TextureVK* m_Texture = nullptr;
		};
	}
}
