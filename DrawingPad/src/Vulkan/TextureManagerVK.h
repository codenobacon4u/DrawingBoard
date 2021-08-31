#pragma once
#include "TextureManager.h"

namespace VkAPI {
	class GraphicsDeviceVK;
	class TextureManagerVK : public TextureManager
	{
	public:
		TextureManagerVK(GraphicsDevice* device);

	private:
		virtual Texture* CreateTexture(TextureFormat format, unsigned char* data, uint32_t width, uint32_t height, uint32_t channels) override;
	};
}