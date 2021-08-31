#pragma once

#include "Texture.h"

#include <unordered_map>

class GraphicsDevice;
class TextureManager
{
public:
	TextureManager(GraphicsDevice* device)
		: m_Device(device)
	{}

	Texture* GetTexture(const std::string& path, TextureFormat format);
protected:
	virtual Texture* CreateTexture(TextureFormat format, unsigned char* data, uint32_t width, uint32_t height, uint32_t channels) = 0;

protected:
	GraphicsDevice* m_Device;
	std::unordered_map<uint64_t, Texture*> m_Textures;
};

