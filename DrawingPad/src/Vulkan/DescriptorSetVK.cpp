#include "pwpch.h"
#include "DescriptorSetVK.h"
#include "GraphicsDeviceVK.h"

namespace Vulkan {
	DescriptorSetPoolVK::DescriptorSetPoolVK(GraphicsDeviceVK* device)
		: m_Device(device)
	{
	}

	DescriptorSetPoolVK::~DescriptorSetPoolVK()
	{
		for (auto pool : m_FreePools)
			if (pool != VK_NULL_HANDLE)
				vkDestroyDescriptorPool(m_Device->Get(), pool, nullptr);
		for (auto pool : m_UsedPools)
			if (pool != VK_NULL_HANDLE)
				vkDestroyDescriptorPool(m_Device->Get(), pool, nullptr);
	}

	std::pair<bool, VkDescriptorSet> DescriptorSetPoolVK::RequestDescriptorSet(VkDescriptorSetLayout layout, const DSLKey& hash, const uint32_t* bindStages)
	{
		bool found = true;
		if (m_CurrPool == VK_NULL_HANDLE)
		{
			m_CurrPool = GetPool();
			m_UsedPools.push_back(m_CurrPool);
		}

		VkDescriptorSetAllocateInfo info = {};
		info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		info.pSetLayouts = &layout;
		info.descriptorPool = m_CurrPool;
		info.descriptorSetCount = 1;

		VkDescriptorSet descriptor;
		VkResult result = vkAllocateDescriptorSets(m_Device->Get(), &info, &descriptor);

		auto it = m_Layouts.find(hash);
		if (it == m_Layouts.end())
		{
			found = false;
			it = m_Layouts.insert(std::make_pair(hash, std::move(layout))).first;
		}

		switch (result)
		{
		case VK_SUCCESS:
			return { found, descriptor };
			break;
		case VK_ERROR_FRAGMENTED_POOL:
		case VK_ERROR_OUT_OF_POOL_MEMORY:
			m_CurrPool = GetPool();
			m_UsedPools.push_back(m_CurrPool);
			if (vkAllocateDescriptorSets(m_Device->Get(), &info, &descriptor) == VK_SUCCESS)
				return { found, descriptor };
			break;
		default:
			return { false, VK_NULL_HANDLE };
			break;
		}

		return { false, VK_NULL_HANDLE };
	}

	VkDescriptorPool DescriptorSetPoolVK::GetPool()
	{
		if (m_FreePools.size() > 0)
		{
			auto pool = m_FreePools.back();
			m_FreePools.pop_back();
			return pool;
		}
		else
			return CreatePool();
	}

	void DescriptorSetPoolVK::ResetPools()
	{
		for (auto pool : m_UsedPools)
			vkResetDescriptorPool(m_Device->Get(), pool, 0);

		m_FreePools = m_UsedPools;
		m_UsedPools.clear();
		m_CurrPool = VK_NULL_HANDLE;
	}

	VkDescriptorPool DescriptorSetPoolVK::CreatePool()
	{
		std::vector<VkDescriptorPoolSize> sizes;
		sizes.reserve(m_DescSizes.sizes.size());
		for (auto size : m_DescSizes.sizes)
			sizes.push_back({ size.first, uint32_t(size.second * 1000) });
		VkDescriptorPoolCreateInfo info = {};
		info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
		info.maxSets = 1000;
		info.poolSizeCount = (uint32_t)sizes.size();
		info.pPoolSizes = sizes.data();

		VkDescriptorPool pool;
		vkCreateDescriptorPool(m_Device->Get(), &info, nullptr, &pool);

		return pool;
	}

	DescriptorSetLayoutCacheVK::DescriptorSetLayoutCacheVK(GraphicsDeviceVK* device)
		: m_Device(device)
	{
	}

	DescriptorSetLayoutCacheVK::~DescriptorSetLayoutCacheVK()
	{
		for (const auto& [key, layout] : m_Array)
			vkDestroyDescriptorSetLayout(m_Device->Get(), layout, nullptr);
		for (const auto& [key, layout] : m_Layout)
			vkDestroyDescriptorSetLayout(m_Device->Get(), layout, nullptr);
	}

	VkDescriptorSetLayout DescriptorSetLayoutCacheVK::GetLayout(const std::vector<ResourceBinding> layout, const uint32_t* bindStages)
	{
		DSLKey key = {};
		for (auto resource : layout)
		{
			switch (resource.Type) {
			case ResourceBindingType::ImageSampler:
				key.SampledImages |= 1 << resource.Binding;
				break;
			case ResourceBindingType::ImageStorage:
				key.StorageImages |= 1 << resource.Binding;
				break;
			case ResourceBindingType::UniformBuffer:
				key.UniformBuffers |= 1 << resource.Binding;
				break;
			case ResourceBindingType::StorageBuffer:
				key.StorageBuffers |= 1 << resource.Binding;
				break;
			case ResourceBindingType::InputAttachment:
				key.InputAttachments |= 1 << resource.Binding;
				break;
			case ResourceBindingType::Sampler:
				key.Samplers |= 1 << resource.Binding;
				break;
			case ResourceBindingType::Image:
				key.SeparateImages |= 1 << resource.Binding;
				break;
			}
		}
		//key.Stages = bindStages;
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
			CreateNewLayout(key, bindStages, &descLayout);
			it = m_Layout.insert(std::make_pair(key, std::move(descLayout))).first;
		}

		return it->second;
#endif
	}

	VkDescriptorSetLayout DescriptorSetLayoutCacheVK::GetLayout(const DSLKey& layout, const uint32_t* bindStages)
	{
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
		auto it = m_Layout.find(layout);
		if (it == m_Layout.end())
		{
			VkDescriptorSetLayout descLayout;
			CreateNewLayout(layout, bindStages, &descLayout);
			it = m_Layout.insert(std::make_pair(layout, std::move(descLayout))).first;
		}

		return it->second;
#endif
	}

	void DescriptorSetLayoutCacheVK::CreateNewLayout(const DSLKey& layout, const uint32_t* bindStages, VkDescriptorSetLayout* descLayout)
	{
		std::vector<VkDescriptorSetLayoutBinding> bindings;
		VkDescriptorSetLayoutCreateInfo createInfo = {};
		createInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;

		for (uint32_t i = 0; i < 32; i++)
			if (layout.SampledImages & 1 << i)
				bindings.push_back({ i, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, bindStages[i], 0 });
		for (uint32_t i = 0; i < 32; i++)
			if (layout.StorageImages & 1 << i)
				bindings.push_back({ i, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1, bindStages[i], 0 });
		for (uint32_t i = 0; i < 32; i++)
			if (layout.UniformBuffers & 1 << i)
				bindings.push_back({ i, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, bindStages[i], 0 });
		for (uint32_t i = 0; i < 32; i++)
			if (layout.StorageBuffers & 1 << i)
				bindings.push_back({ i, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1, bindStages[i], 0 });
		for (uint32_t i = 0; i < 32; i++)
			if (layout.InputAttachments & 1 << i)
				bindings.push_back({ i, VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1, bindStages[i], 0 });
		for (uint32_t i = 0; i < 32; i++)
			if (layout.Samplers & 1 << i)
				bindings.push_back({ i, VK_DESCRIPTOR_TYPE_SAMPLER, 1, bindStages[i], 0 });
		for (uint32_t i = 0; i < 32; i++)
			if (layout.SeparateImages & 1 << i)
				bindings.push_back({ i, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1, bindStages[i], 0 });

		createInfo.bindingCount = static_cast<uint32_t>(bindings.size());
		createInfo.pBindings = bindings.data();

		vkCreateDescriptorSetLayout(m_Device->Get(), &createInfo, nullptr, descLayout);
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
			Hasher h;
			h.AddHash(SampledImages);
			h.AddHash(StorageImages);
			h.AddHash(UniformBuffers);
			h.AddHash(StorageBuffers);
			h.AddHash(SampledBuffers);
			h.AddHash(InputAttachments);
			h.AddHash(Samplers);
			h.AddHash(SeparateImages);
			h.AddHash(Stages);
			Hash = h.Get();
		}
		return Hash;
	}
}