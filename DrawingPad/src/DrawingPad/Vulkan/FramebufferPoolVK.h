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

			void DeleteViewEntry(VkImageView view);

			VkFramebuffer GetFramebuffer(const FBKey& key, uint32_t width, uint32_t height, uint32_t layers);

		private:
			GraphicsDeviceVK* m_Device;

			struct FBKeyHash {
				std::size_t operator()(const FBKey& key) const { return key.GetHash(); }
			};

			std::unordered_map<FBKey, VkFramebuffer, FBKeyHash> m_Map = {};
			std::unordered_multimap<VkImageView, FBKey> m_VTKMap = {};
		};
	}
}
