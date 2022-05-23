#pragma once
#include "Framebuffer.h"

#include <vulkan/vulkan.h>

namespace Vulkan
{
	class GraphicsDeviceVK;
	class FramebufferVK : public Framebuffer
	{
	public:
		FramebufferVK(GraphicsDeviceVK* device, const FramebufferDesc& desc);

		VkFramebuffer Get() const { return m_Buffer; }

	private:
		VkFramebuffer m_Buffer;
	};
}
