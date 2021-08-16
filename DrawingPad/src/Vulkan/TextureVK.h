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
		TextureVK(GraphicsDeviceVK* device, const TextureDesc& desc, const void* data = nullptr);
		TextureVK(GraphicsDeviceVK* device, const TextureDesc& desc, VkImage image);

		~TextureVK();

		VkImage GetImage() { return m_Image; }

		TextureView* CreateView(const TextureViewDesc& desc);

	private:
		GraphicsDeviceVK* m_Device;
		const void* m_Data;
		VkImage m_Image;
		VkDeviceMemory m_Mem;
		TextureViewVK* m_DefaultView = nullptr;
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
