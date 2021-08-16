#pragma once

#include "GraphicsDevice.h"

class TextureManager
{
public:
	TextureManager(GraphicsDevice* device)
	{}

	Texture* GetTexture(const std::string& path, TextureFormat format);
private:
	virtual Texture* CreateTexture() = 0;
};

