#pragma once

#include "RenderPass.h"

typedef struct FramebufferDesc {
	RenderPass* RenderPass = nullptr;
	uint32_t AttachmentCount;
	TextureView* const* Attachments = nullptr;
	uint32_t Width = 0;
	uint32_t Height = 0;
	uint32_t ArraySlices = 0;
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

