#pragma once
#include "Texture.h"
#include <vulkan/vulkan.h>

namespace VkAPI
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
		void TransistionLayout(VkImageLayout oldLayout, VkImageLayout newLayout);
		void CopyFromBuffer(VkBuffer buffer, uint32_t width, uint32_t height);

	private:
		GraphicsDeviceVK* m_Device;
		const void* m_Data;
		VkImage m_Image;
		VkDeviceMemory m_Mem;
		VkSampler m_Sampler;
	};

	class TextureViewVK : public TextureView
	{
	public:
		TextureViewVK(GraphicsDeviceVK* device, const TextureViewDesc& desc, TextureVK* texture, VkImageAspectFlags aspectFlags);
		VkImageView GetView() { return m_View; }

		~TextureViewVK();

		virtual TextureVK* GetTexture() override { return m_Texture; }

	private:
		GraphicsDeviceVK* m_Device;
		TextureVK* m_Texture;
		VkImageView m_View;
	};
}
