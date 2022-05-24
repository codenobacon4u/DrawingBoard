#pragma once

#include "RenderPass.h"

typedef struct FramebufferDesc {
	std::vector<TextureView*> Attachments = {};
	uint32_t Width = 0;
	uint32_t Height = 0;
	uint32_t Layers = 0;
	RenderPass* RenderPass = nullptr;
} FramebufferDesc;

class Framebuffer
{
public:
	Framebuffer(GraphicsDevice* device, const FramebufferDesc& desc) 
		: m_Device(device), m_Desc(desc)
	{}

	virtual ~Framebuffer() {}

	const FramebufferDesc& GetDesc() { return m_Desc; }

protected:
	GraphicsDevice* m_Device;
	FramebufferDesc m_Desc;
};

