#pragma once

#include <unordered_map>

#include <vulkan/vulkan.h>

#include "StructsVK.h"

#include "DrawingPad/Shader.h"

namespace DrawingPad
{
	namespace Vulkan
	{
		class GraphicsDeviceVK;
		class DescriptorSetPoolVK
		{
		public:
			DescriptorSetPoolVK(GraphicsDeviceVK* device);

			~DescriptorSetPoolVK();

			std::pair<bool, VkDescriptorSet> RequestDescriptorSet(VkDescriptorSetLayout layout, size_t hash);
			VkDescriptorPool Get();

			void Reset();

		private:
			VkDescriptorPool CreatePool();

		private:
			GraphicsDeviceVK* m_Device = nullptr;

			VkDescriptorPool m_Handle = VK_NULL_HANDLE;
			PoolSizes m_DescSizes;
			std::vector<VkDescriptorPool> m_FreePools;
			std::vector<VkDescriptorPool> m_UsedPools;
			std::unordered_map<size_t, VkDescriptorSet> m_DescriptorSets;
		};

		class DescriptorSetLayoutCacheVK
		{
		public:
			DescriptorSetLayoutCacheVK(GraphicsDeviceVK* device);

			~DescriptorSetLayoutCacheVK();

			VkDescriptorSetLayout GetLayout(const std::vector<ShaderResourceBinding> layout);

		private:
			void CreateNewLayout(const DSLKey& layout, VkDescriptorSetLayout* descLayout);
			struct DSLKeyHash {
				std::size_t operator()(const DSLKey& key) const { return key.GetHash(); }
			};
		private:
			GraphicsDeviceVK* m_Device = nullptr;
			std::unordered_map<DSLKey, VkDescriptorSetLayout, DSLKeyHash> m_Layout = {};
		};
	}
}
