#include "pwpch.h"
#include "DescriptorSetVK.h"
#include "GraphicsDeviceVK.h"

namespace VkAPI {
	DescriptorSetPoolVK::DescriptorSetPoolVK(GraphicsDeviceVK* device)
		: m_Device(device)
	{
	}

	VkDescriptorSetLayout DescriptorSetPoolVK::GetLayout(const DescriptorSetLayout& layout, const uint32_t* bindStages)
	{
		DSLKey key = {};
		for (auto image : layout.SampledImages)
			key.SampledImages |= 1 << image.Binding;
		for (auto image : layout.StorageImages)
			key.StorageImages |= 1 << image.Binding;
		for (auto buffer : layout.UniformBuffers)
			key.UniformBuffers |= 1 << buffer.Binding;
		for (auto buffer : layout.StorageBuffers)
			key.StorageBuffers |= 1 << buffer.Binding;
		for (auto buffer : layout.SampledBuffers)
			key.SampledBuffers |= 1 << buffer.Binding;
		for (auto input : layout.InputAttachments)
			key.InputAttachments |= 1 << input.Binding;
		for (auto sampler : layout.Samplers)
			key.Samplers |= 1 << sampler.Binding;
		for (auto image : layout.SeparateImages)
			key.SeparateImages |= 1 << image.Binding;
		key.Stages = bindStages;
#if 0
		auto it = m_Array.begin();
		for (; it != m_Array.end(); it++) {
			if (it->first == key)
				return it->second;
		}
		if (it == m_Array.end())
		{
			VkDescriptorSetLayout descLayout;
			CreateNewLayout(layout, bindStages, &descLayout);
			m_Array.emplace_back(s)
		}
#else
		auto it = m_Layout.find(key);
		if (it == m_Layout.end())
		{
			VkDescriptorSetLayout descLayout;
			CreateNewLayout(layout, bindStages, &descLayout);
			it = m_Layout.insert(std::make_pair(key, std::move(descLayout))).first;
		}

		return it->second;
#endif
	}

	void DescriptorSetPoolVK::CreateNewLayout(const DescriptorSetLayout& layout, const uint32_t* bindStages, VkDescriptorSetLayout* descLayout)
	{
		std::vector<VkDescriptorSetLayoutBinding> bindings;
		m_PoolSizes.resize(8);
		VkDescriptorSetLayoutCreateInfo createInfo = {};
		createInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
		for (auto image : layout.SampledImages)
		{
			uint32_t stages = bindStages[image.Binding];
			if (stages == 0)
				continue;
			bindings.push_back({ image.Binding, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, stages, nullptr });
			m_PoolSizes[0].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
			m_PoolSizes[0].descriptorCount += 1;
		}
		for (auto buffer : layout.SampledBuffers)
		{
			uint32_t stages = bindStages[buffer.Binding];
			if (stages == 0)
				continue;
			bindings.push_back({ buffer.Binding, VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 1, stages, nullptr });
			m_PoolSizes[1].type = VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER;
			m_PoolSizes[1].descriptorCount += 1;
		}
		for (auto image : layout.StorageImages)
		{
			uint32_t stages = bindStages[image.Binding];
			if (stages == 0)
				continue;
			bindings.push_back({ image.Binding, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1, stages, nullptr });
			m_PoolSizes[2].type = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
			m_PoolSizes[2].descriptorCount += 1;
		}
		for (auto buffer : layout.StorageBuffers)
		{
			uint32_t stages = bindStages[buffer.Binding];
			if (stages == 0)
				continue;
			bindings.push_back({ buffer.Binding, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1, stages, nullptr });
			m_PoolSizes[3].type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
			m_PoolSizes[3].descriptorCount += 1;
		}
		for (auto buffer : layout.UniformBuffers)
		{
			uint32_t stages = bindStages[buffer.Binding];
			if (stages == 0)
				continue;
			bindings.push_back({ buffer.Binding, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1, stages, nullptr });
			m_PoolSizes[4].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
			m_PoolSizes[4].descriptorCount += 1;
		}
		for (auto input : layout.InputAttachments)
		{
			uint32_t stages = bindStages[input.Binding];
			if (stages == 0)
				continue;
			bindings.push_back({ input.Binding, VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1, stages, nullptr });
			m_PoolSizes[5].type = VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT;
			m_PoolSizes[5].descriptorCount += 1;
		}
		for (auto image : layout.SeparateImages)
		{
			uint32_t stages = bindStages[image.Binding];
			if (stages == 0)
				continue;
			bindings.push_back({ image.Binding, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1, stages, nullptr });
			m_PoolSizes[6].type = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
			m_PoolSizes[6].descriptorCount += 1;
		}
		for (auto sampler : layout.Samplers)
		{
			uint32_t stages = bindStages[sampler.Binding];
			if (stages == 0)
				continue;
			bindings.push_back({ sampler.Binding, VK_DESCRIPTOR_TYPE_SAMPLER, 1, stages, nullptr });
			m_PoolSizes[7].type = VK_DESCRIPTOR_TYPE_SAMPLER;
			m_PoolSizes[7].descriptorCount += 1;
		}

		createInfo.bindingCount = static_cast<uint32_t>(bindings.size());
		createInfo.pBindings = bindings.data();

		vkCreateDescriptorSetLayout(m_Device->Get(), &createInfo, nullptr, descLayout);

		VkDescriptorSetAllocateInfo allocInfo = {};
		allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		allocInfo.descriptorPool = m_;
	}

	bool DSLKey::operator==(const DSLKey& rhs) const
	{
		if (GetHash() != rhs.GetHash() || 
			SampledImages != rhs.SampledImages || 
			StorageImages != rhs.StorageImages || 
			UniformBuffers != rhs.UniformBuffers || 
			StorageBuffers != rhs.StorageBuffers || 
			SampledBuffers != rhs.SampledBuffers || 
			InputAttachments != rhs.InputAttachments ||
			Samplers != rhs.Samplers ||
			SeparateImages != rhs.SeparateImages ||
			Stages != rhs.Stages)
			return false;
		return true;
	}

	size_t DSLKey::GetHash() const
	{
		if (Hash == 0)
		{
			hash_combine(Hash, SampledImages);
			hash_combine(Hash, StorageImages);
			hash_combine(Hash, UniformBuffers);
			hash_combine(Hash, StorageBuffers);
			hash_combine(Hash, SampledBuffers);
			hash_combine(Hash, InputAttachments);
			hash_combine(Hash, Samplers);
			hash_combine(Hash, SeparateImages);
			hash_combine(Hash, Stages);
		}
		return Hash;
	}
}