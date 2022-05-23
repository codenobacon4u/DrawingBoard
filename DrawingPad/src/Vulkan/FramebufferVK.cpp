#include "pwpch.h"
#include "FramebufferVK.h"
#include "GraphicsDeviceVK.h"
#include "RenderPassVK.h"

namespace Vulkan {
	FramebufferVK::FramebufferVK(GraphicsDeviceVK* device, const FramebufferDesc& desc)
		: Framebuffer(device, desc)
	{
		VkFramebufferCreateInfo createInfo = {};
		createInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		createInfo.pNext = nullptr;
		createInfo.flags = 0;
		createInfo.renderPass = ((RenderPassVK*)desc.RenderPass)->GetRenderPass();

		createInfo.attachmentCount = desc.AttachmentCount;
		std::vector<VkImageView> imgViews(desc.AttachmentCount);
		for (uint32_t i = 0; i < desc.AttachmentCount; i++)
			if (auto* view = desc.Attachments[i])
				imgViews[i] = ((TextureViewVK*)view)->GetView();
		createInfo.pAttachments = imgViews.data();

		createInfo.width = desc.Width;
		createInfo.height = desc.Height;
		createInfo.layers = desc.ArraySlices;

		vkCreateFramebuffer(device->Get(), &createInfo, nullptr, &m_Buffer);
	}
}
