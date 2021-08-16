#include "pwpch.h"
#include "FramebufferPoolVK.h"

#include "GraphicsDeviceVK.h"

#include <chrono>

namespace VkAPI {
	using FloatingPointMicroseconds = std::chrono::duration<double, std::micro>;
	bool FBKey::operator==(const FBKey& rhs) const
	{
		if (GetHash() != rhs.GetHash() ||
			Pass != rhs.Pass ||
			AttachmentCount != rhs.AttachmentCount ||
			CommandQueueMask != rhs.CommandQueueMask)
			return false;
		for (uint32_t i = 0; i < AttachmentCount; i++)
			if (Attachments[i] != rhs.Attachments[i])
				return false;
		return true;
	}

	size_t FBKey::GetHash() const
	{
		if (Hash == 0)
		{
			Hasher h;
			h.AddHash(Pass);
			h.AddHash(AttachmentCount);
			h.AddHash(CommandQueueMask);
			for (uint32_t i = 0; i < AttachmentCount; i++)
				h.AddHash(Attachments[i]);
			Hash = h.Get();
		}
		return Hash;
	}

	void FramebufferPoolVK::DeleteViewEntry(VkImageView view)
	{
		auto range = m_VTKMap.equal_range(view);
		for (auto it = range.first; it != range.second; it++)
		{
			auto fb = m_Map.find(it->second);
			if (fb != m_Map.end())
			{
				vkDestroyFramebuffer(m_Device.Get(), fb->second, nullptr);
				m_Map.erase(fb);
			}
		}
		m_VTKMap.erase(range.first, range.second);
	}

	VkFramebuffer FramebufferPoolVK::GetFramebuffer(const FBKey& key, uint32_t width, uint32_t height, uint32_t layers)
	{
#if 1
		auto it = m_Map.find(key);
		if (it != m_Map.end())
			return it->second;
		else
		{
			VkFramebufferCreateInfo createInfo = {};
			createInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
			createInfo.flags = 0;
			createInfo.renderPass = key.Pass;
			createInfo.attachmentCount = key.AttachmentCount;
			createInfo.pAttachments = key.Attachments;
			createInfo.width = width;
			createInfo.height = height;
			createInfo.layers = layers;
			VkFramebuffer fb;
			vkCreateFramebuffer(m_Device.Get(), &createInfo, nullptr, &fb);
			m_Map.insert(std::make_pair(key, std::move(fb)));

			for (uint32_t i = 0; i < key.AttachmentCount; i++)
				m_VTKMap.emplace(key.Attachments[i], key);

			return fb;
		}
#else
		auto it = m_Array.begin();
		for (; it != m_Array.end(); it++) {
			if (it->first == key)
				return it->second;
		}
		if (it == m_Array.end())
		{
			VkFramebufferCreateInfo createInfo = {};
			createInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
			createInfo.flags = 0;
			createInfo.renderPass = key.Pass;
			createInfo.attachmentCount = key.AttachmentCount;
			createInfo.pAttachments = key.Attachments;
			createInfo.width = width;
			createInfo.height = height;
			createInfo.layers = layers;
			VkFramebuffer fb;
			vkCreateFramebuffer(m_Device.Get(), &createInfo, nullptr, &fb);
			m_Array.emplace_back(std::make_pair(key, std::move(fb)));

			for (uint32_t i = 0; i < key.AttachmentCount; i++)
				m_VTKMap.emplace(key.Attachments[i], key);

			return fb;
		}
#endif
	}
}