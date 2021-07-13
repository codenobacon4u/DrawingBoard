#pragma once

#include "FramebufferVK.h"

#include <vulkan/vulkan.h>
#include <unordered_map>

namespace VkAPI {

	typedef struct FBKey {
		VkRenderPass Pass;
		uint32_t AttachmentCount;
		VkImageView Attachments[9];
		uint64_t CommandQueueMask;

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

	template <typename T>
	inline void hash_combine(std::size_t& s, const T& v)
	{
		std::hash<T> h;
		s ^= h(v) + 0x9e3779b9 + (s << 6) + (s >> 2);
	}
}
