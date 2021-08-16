#pragma once
#include "Texture.h"

typedef struct SwapchainDesc {
	uint32_t Width = 0;
	uint32_t Height = 0;
	TextureFormat ColorFormat = TextureFormat::RGBA8UnormSRGB;
	TextureFormat DepthFormat = TextureFormat::D32Float;
	uint32_t BufferCount = 2;
} SwapchainDesc;

class Swapchain {
public:

	virtual ~Swapchain() {}

	virtual void Resize(uint32_t width, uint32_t height) = 0;
	virtual void Present(uint32_t sync) = 0;
	virtual void* GetNative() = 0;
	virtual uint32_t GetImageIndex() = 0;
	virtual std::pair<uint32_t, TextureView*> GetNextBackbuffer() = 0;
	virtual TextureView* GetDepthBufferView() = 0;
	
	void SetVSync(bool enabled) { m_VSync = enabled; }
	bool IsVSync() { return m_VSync; }

	SwapchainDesc GetDesc() { return m_Desc; }

protected:
	Swapchain(const SwapchainDesc& desc)
		: m_Desc(desc) {}

protected:
	bool m_VSync;
	SwapchainDesc m_Desc;
};