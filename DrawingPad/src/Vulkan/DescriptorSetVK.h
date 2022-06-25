#pragma once

#include <unordered_map>

#include <vulkan/vulkan.h>

#include "Shader.h"

namespace Vulkan {

	struct DSLKey {
		std::vector<ShaderResourceBinding> Resources;

		DSLKey(std::vector<ShaderResourceBinding> resources);

		bool operator==(const DSLKey& rhs) const;
		size_t GetHash() const;
	private:
		mutable size_t Hash = 0;
	};

	struct PoolSizes {
		std::vector<std::pair<VkDescriptorType, float>> sizes =
		{
			{ VK_DESCRIPTOR_TYPE_SAMPLER, 0.5f },
			{ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 4.f },
			{ VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 4.f },
			{ VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1.f },
			{ VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 1.f },
			{ VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 1.f },
			{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 2.f },
			{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 2.f },
			{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1.f },
			{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1.f },
			{ VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 0.5f }
		};
	};

	class GraphicsDeviceVK;
	class DescriptorSetPoolVK;

	class DescriptorSetPoolVK
	{
	public:
		DescriptorSetPoolVK(GraphicsDeviceVK* device);

		~DescriptorSetPoolVK();

		std::pair<bool, VkDescriptorSet> RequestDescriptorSet(VkDescriptorSetLayout layout, size_t hash);
		VkDescriptorPool GetPool();

		void ResetPools();

	private:
		VkDescriptorPool CreatePool();

	private:
		GraphicsDeviceVK* m_Device = nullptr;
		
		VkDescriptorPool m_CurrPool = VK_NULL_HANDLE;
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
