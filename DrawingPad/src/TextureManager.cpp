#include "pwpch.h"
#include "TextureManager.h"
#include "Utils.h"

#include <stb_image.h>

Texture* TextureManager::GetTexture(const std::string& path, TextureFormat format)
{
	Hasher hash;
	hash.AddHash(path);
	hash.AddHash(format);

	auto it = m_Textures.find(hash.Get());
	if (it != m_Textures.end())
		return it->second;

	int texWidth, texHeight, texChannels;
	stbi_uc* pixels = stbi_load(path.c_str(), &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);

	if (!pixels) {
		throw std::runtime_error("Failed to load texture image at \"" + path + "\"");
	}

	auto ret = m_Textures.insert(std::make_pair(hash.Get(), CreateTexture(format, (unsigned char*)pixels, texWidth, texHeight, texChannels)));
	stbi_image_free(pixels);
	return ret.first->second;
}
