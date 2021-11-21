#pragma once

#include "FramebufferVK.h"

#include <vulkan/vulkan.h>
#include <unordered_map>

namespace VkAPI {

	typedef struct FBKey {
		VkRenderPass Pass = VK_NULL_HANDLE;
		uint32_t AttachmentCount = 0;
		VkImageView Attachments[9] = {};
		uint64_t CommandQueueMask = 0;

		bool operator==(const FBKey& rhs) const;
		size_t GetHash() const;

	private:
		mutable size_t Hash = 0;
	} FBKey;

	class GraphicsDeviceVK;
	class FramebufferPoolVK
	{
	public:
		FramebufferPoolVK(GraphicsDeviceVK& device)
			: m_Device(device)
		{}

		~FramebufferPoolVK();

		void DeleteViewEntry(VkImageView view);

		VkFramebuffer GetFramebuffer(const FBKey& key, uint32_t width, uint32_t height, uint32_t layers);

	private:
		GraphicsDeviceVK& m_Device;

		struct FBKeyHash {
			std::size_t operator()(const FBKey& key) const { return key.GetHash(); }
		};

		std::vector<std::pair<FBKey, VkFramebuffer>> m_Array;
		std::unordered_map<FBKey, VkFramebuffer, FBKeyHash> m_Map = {};
		std::unordered_multimap<VkImageView, FBKey> m_VTKMap = {};
	};
}
