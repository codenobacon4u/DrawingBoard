#pragma once

#include "Shader.h"

#include <vulkan/vulkan.h>
#include <unordered_map>

namespace VkAPI {

	struct DSLKey {
		uint32_t SampledImages = 0;
		uint32_t StorageImages = 0;
		uint32_t UniformBuffers = 0;
		uint32_t StorageBuffers = 0;
		uint32_t SampledBuffers = 0;
		uint32_t InputAttachments = 0;
		uint32_t Samplers = 0;
		uint32_t SeparateImages = 0;
		//uint32_t Floats;
		const uint32_t* Stages = nullptr;

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
	class DescriptorSetPoolVK
	{
	public:
		DescriptorSetPoolVK(GraphicsDeviceVK* device);

		~DescriptorSetPoolVK();

		std::pair<bool, VkDescriptorSet> RequestDescriptorSet(VkDescriptorSetLayout layout, const DSLKey& hash, const uint32_t* bindStages);
		VkDescriptorPool GetPool();

		void ResetPools();

	private:
		VkDescriptorPool CreatePool();
		struct DSLKeyHash {
			std::size_t operator()(const DSLKey& key) const { return key.GetHash(); }
		};
	private:
		GraphicsDeviceVK* m_Device = nullptr;
		
		VkDescriptorPool m_CurrPool = VK_NULL_HANDLE;
		PoolSizes m_DescSizes;
		std::vector<VkDescriptorPool> m_FreePools;
		std::vector<VkDescriptorPool> m_UsedPools;
		std::unordered_map<DSLKey, VkDescriptorSetLayout, DSLKeyHash> m_Layouts = {};
	};

	class DescriptorSetLayoutCacheVK
	{
	public:
		DescriptorSetLayoutCacheVK(GraphicsDeviceVK* device);

		~DescriptorSetLayoutCacheVK();

		VkDescriptorSetLayout GetLayout(const std::vector<ResourceBinding> layout, const uint32_t* bindStages);
		VkDescriptorSetLayout GetLayout(const DSLKey& layout, const uint32_t* bindStages);

	private:
		void CreateNewLayout(const DSLKey& layout, const uint32_t* bindStages, VkDescriptorSetLayout* descLayout);
		struct DSLKeyHash {
			std::size_t operator()(const DSLKey& key) const { return key.GetHash(); }
		};
	private:
		GraphicsDeviceVK* m_Device = nullptr;
		std::vector<std::pair<DSLKey, VkDescriptorSetLayout>> m_Array;
		std::unordered_map<DSLKey, VkDescriptorSetLayout, DSLKeyHash> m_Layout = {};
	};
}