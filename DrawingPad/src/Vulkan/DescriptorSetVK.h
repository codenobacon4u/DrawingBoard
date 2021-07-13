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

	class GraphicsDeviceVK;
	class DescriptorSetPoolVK
	{
	public:
		DescriptorSetPoolVK(GraphicsDeviceVK* device);

		VkDescriptorSetLayout GetLayout(const DescriptorSetLayout& layout, const uint32_t* bindStages);

	private:
		void CreateNewLayout(const DescriptorSetLayout& layout, const uint32_t* bindStages, VkDescriptorSetLayout* descLayout);
		struct DSLKeyHash {
			std::size_t operator()(const DSLKey& key) const { return key.GetHash(); }
		};
	private:
		GraphicsDeviceVK* m_Device = nullptr;
		std::vector<std::pair<DSLKey, VkDescriptorSetLayout>> m_Array;
		std::unordered_map<DSLKey, VkDescriptorSetLayout, DSLKeyHash> m_Layout = {};
		std::vector<VkDescriptorPoolSize> m_PoolSizes;
		VkDescriptorPool m_Pool = VK_NULL_HANDLE;
	};
}