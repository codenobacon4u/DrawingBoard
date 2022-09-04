#pragma once

#include <unordered_map>
#include <vector>

#include <vulkan/vulkan.h>

#include "StructsVK.h"
namespace DrawingPad
{
	namespace Vulkan
	{
		class GraphicsDeviceVK;
		class FramebufferPoolVK
		{
		public:
			FramebufferPoolVK(GraphicsDeviceVK* device)
				: m_Device(device)
			{}

			~FramebufferPoolVK();

			VkFramebuffer GetFramebuffer(const FBKey& key, uint32_t layers);

		private:
			GraphicsDeviceVK* m_Device;

			struct FBKeyHash {
				std::size_t operator()(const FBKey& key) const { return key.GetHash(); }
			};

			std::unordered_map<FBKey, VkFramebuffer, FBKeyHash> m_Map = {};
		};
	}
}
