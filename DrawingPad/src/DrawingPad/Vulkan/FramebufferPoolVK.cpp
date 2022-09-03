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

		VkFramebuffer FramebufferPoolVK::GetFramebuffer(const FBKey& key, uint32_t width, uint32_t height, uint32_t layers)
		{
			auto it = m_Map.find(key);
			if (it != m_Map.end())
				return it->second;
			else
			{
				std::vector<VkFramebufferAttachmentImageInfo> attachInfos(key.Formats.size());
				for (size_t i = 0; i < key.Formats.size(); i++)
				{
					attachInfos[0].sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_ATTACHMENT_IMAGE_INFO;
					attachInfos[0].width = width;
					attachInfos[0].height = height;
					attachInfos[0].layerCount = layers;
					attachInfos[0].viewFormatCount = 1;
					attachInfos[0].pViewFormats = &key.Formats[0];
					attachInfos[0].usage = key.Usages[0];
				}

				VkFramebufferAttachmentsCreateInfo attachCreateInfo = {};
				attachCreateInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_ATTACHMENTS_CREATE_INFO;
				attachCreateInfo.attachmentImageInfoCount = static_cast<uint32_t>(attachInfos.size());
				attachCreateInfo.pAttachmentImageInfos = attachInfos.data();

				VkFramebufferCreateInfo createInfo = {};
				createInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
				createInfo.pNext = &attachCreateInfo;
				createInfo.flags |= VK_FRAMEBUFFER_CREATE_IMAGELESS_BIT;
				createInfo.renderPass = key.Pass;
				createInfo.width = width;
				createInfo.height = height;
				createInfo.layers = layers;

				VkFramebuffer fb;
				vkCreateFramebuffer(m_Device->Get(), &createInfo, nullptr, &fb);
				m_Map.insert(std::make_pair(key, std::move(fb)));

				return fb;
			}
		}
	}
}
