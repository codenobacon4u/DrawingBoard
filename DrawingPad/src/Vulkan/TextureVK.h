#pragma once
#include "Texture.h"
#include <vulkan/vulkan.h>

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

		VkImage GetImage() { return m_Image; }
		VkSampler GetSampler() { return m_Sampler; }

		virtual TextureView* CreateView(const TextureViewDesc& desc) override;

	private:
		void TransistionLayout(VkCommandBuffer cmd, VkImageLayout oldLayout, VkImageLayout newLayout);
		void CopyFromBuffer(VkCommandBuffer cmd, VkBuffer buffer, uint32_t width, uint32_t height);
		void GenerateMipmaps(VkCommandBuffer cmd);
	private:
		GraphicsDeviceVK* m_Device = nullptr;
		const void* m_Data = nullptr;
		VkImage m_Image = VK_NULL_HANDLE;
		VkDeviceMemory m_Mem = VK_NULL_HANDLE;
		VkSampler m_Sampler = VK_NULL_HANDLE;
	};

	class TextureViewVK : public TextureView
	{
	public:
		TextureViewVK(GraphicsDeviceVK* device, const TextureViewDesc& desc, TextureVK* texture, VkImageAspectFlags aspectFlags);
		VkImageView GetView() { return m_View; }

		~TextureViewVK();

		virtual TextureVK* GetTexture() override { return m_Texture; }

	private:
		GraphicsDeviceVK* m_Device = nullptr;
		TextureVK* m_Texture = nullptr;
		VkImageView m_View = VK_NULL_HANDLE;
	};
}
