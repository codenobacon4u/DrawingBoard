#include "dppch.h"
#include "FramebufferPoolVK.h"

#include <chrono>

#include "GraphicsDeviceVK.h"

namespace DrawingPad
{
	namespace Vulkan
	{
		using FloatingPointMicroseconds = std::chrono::duration<double, std::micro>;
		bool FBKey::operator==(const FBKey& rhs) const
		{
			if (GetHash() != rhs.GetHash() ||
				Width != rhs.Width ||
				Height != rhs.Height ||
				Formats.size() != rhs.Formats.size() ||
				Usages.size() != rhs.Usages.size())
				return false;
			for (uint32_t i = 0; i < Formats.size(); i++)
				if (Formats[i] != rhs.Formats[i])
					return false;
				else if (Usages[i] != Usages[i])
					return false;
			return true;
		}

		size_t FBKey::GetHash() const
		{
			if (Hash == 0)
			{
				hash_combine(Hash, Width);
				hash_combine(Hash, Height);
				hash_combine(Hash, Formats.size());
				for (uint32_t i = 0; i < Formats.size(); i++) {
					hash_combine(Hash, Formats[i]);
					hash_combine(Hash, Usages[i]);
				}
			}
			return Hash;
		}

		FramebufferPoolVK::~FramebufferPoolVK()
		{
			for (auto& [key, fb] : m_Map)
				vkDestroyFramebuffer(m_Device->Get(), fb, nullptr);
		}

		VkFramebuffer FramebufferPoolVK::GetFramebuffer(const FBKey& key, uint32_t layers)
		{
			auto it = m_Map.find(key);
			if (it != m_Map.end())
				return it->second;
			else
			{
				std::vector<VkFramebufferAttachmentImageInfo> attachInfos(key.Formats.size());
				for (size_t i = 0; i < key.Formats.size(); i++)
				{
					attachInfos[i].sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_ATTACHMENT_IMAGE_INFO;
					attachInfos[i].width = key.Width;
					attachInfos[i].height = key.Height;
					attachInfos[i].layerCount = layers;
					attachInfos[i].viewFormatCount = 1;
					attachInfos[i].pViewFormats = &key.Formats[i];
					attachInfos[i].usage = key.Usages[i];
				}

				VkFramebufferAttachmentsCreateInfo attachCreateInfo = {};
				attachCreateInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_ATTACHMENTS_CREATE_INFO;
				attachCreateInfo.attachmentImageInfoCount = static_cast<uint32_t>(attachInfos.size());
				attachCreateInfo.pAttachmentImageInfos = attachInfos.data();

				VkFramebufferCreateInfo createInfo = {};
				createInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
				createInfo.pNext = &attachCreateInfo;
				createInfo.flags = VK_FRAMEBUFFER_CREATE_IMAGELESS_BIT;
				createInfo.renderPass = key.Pass;
				createInfo.width = key.Width;
				createInfo.height = key.Height;
				createInfo.layers = layers;
				createInfo.attachmentCount = static_cast<uint32_t>(attachInfos.size());

				VkFramebuffer fb;
				vkCreateFramebuffer(m_Device->Get(), &createInfo, nullptr, &fb);
				m_Map.insert(std::make_pair(key, std::move(fb)));

				return fb;
			}
		}
	}
}
