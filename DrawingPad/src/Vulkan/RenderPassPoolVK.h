#pragma once

#include "RenderPassVK.h"

namespace VkAPI 
{
	typedef struct RPKey {
		uint8_t NumColors;
		uint8_t SampleCount;
		TextureFormat ColorFormats[8];
		TextureFormat DepthFormat;

		bool operator==(const RPKey& rhs) const;
		size_t GetHash() const;

	private:
		mutable size_t Hash = 0;
	} RPKey;

	class GraphicsDeviceVK;
	class RenderPassPoolVK
	{
	public:
		RenderPassPoolVK(GraphicsDeviceVK& device)
			: m_Device(device)
		{}

		RenderPassVK* GetRenderPass(const RPKey& key);

	private:
		GraphicsDeviceVK& m_Device;

		struct RPKeyHash {
			std::size_t operator()(const RPKey& key) const { return key.GetHash(); }
		};

		std::vector<std::pair<RPKey, RenderPassVK*>> m_Array;
		std::unordered_map<RPKey, RenderPassVK*, RPKeyHash> m_Map = {};
	};
}
