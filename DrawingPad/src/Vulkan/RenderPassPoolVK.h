#pragma once

#include "RenderPassVK.h"

namespace VkAPI 
{
	typedef struct RPKey {
		uint8_t NumColors = 0;
		uint8_t SampleCount = 1;
		TextureFormat ColorFormats[8] = {};
		TextureFormat DepthFormat = TextureFormat::Unknown;
		bool ClearEnable = true;

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

		~RenderPassPoolVK();

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
