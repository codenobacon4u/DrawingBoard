#include "pwpch.h"
#include "TextureManagerVK.h"
#include "GraphicsDeviceVK.h"

namespace VkAPI {
    TextureManagerVK::TextureManagerVK(GraphicsDevice* device)
        : TextureManager(device)
    {
    }

    Texture* TextureManagerVK::CreateTexture(TextureFormat format, unsigned char* data, uint32_t width, uint32_t height, uint32_t channels)
    {
        TextureDesc desc = {};
        desc.Type = TextureType::DimTex2D;
        desc.Width = width;
        desc.Height = height;
        desc.MipLevels = 1;
        desc.Format = format;
        desc.ArraySize = 1;
        desc.BindFlags = BindFlags::ShaderResource;

        return m_Device->CreateTexture(desc, data);
    }
}