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
		createInfo.renderPass = ((RenderPassVK*)desc.RenderPass)->Get();

		createInfo.attachmentCount = static_cast<uint32_t>(desc.Attachments.size());
		std::vector<VkImageView> attachments = {};
		for (auto attachment : m_Desc.Attachments)
			attachments.push_back(static_cast<TextureViewVK*>(attachment)->GetView());
		createInfo.pAttachments = attachments.data();
		createInfo.width = m_Desc.Width;
		createInfo.height = m_Desc.Height;
		createInfo.layers = m_Desc.Layers;

		if (vkCreateFramebuffer(device->Get(), &createInfo, nullptr, &m_Buffer) != VK_SUCCESS)
			throw std::runtime_error("Failed to create Framebuffer");
	}

	FramebufferVK::~FramebufferVK()
	{
		vkDestroyFramebuffer(((GraphicsDeviceVK*)m_Device)->Get(), m_Buffer, nullptr);
	}
}
