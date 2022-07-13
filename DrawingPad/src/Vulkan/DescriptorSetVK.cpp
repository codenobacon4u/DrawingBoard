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
		for (auto& pool : m_FreePools)
			if (pool != VK_NULL_HANDLE)
				vkDestroyDescriptorPool(m_Device->Get(), pool, nullptr);
		for (auto& pool : m_UsedPools)
			if (pool != VK_NULL_HANDLE)
				vkDestroyDescriptorPool(m_Device->Get(), pool, nullptr);
	}

	std::pair<bool, VkDescriptorSet> DescriptorSetPoolVK::RequestDescriptorSet(VkDescriptorSetLayout layout, size_t hash)
	{
		auto& it = m_DescriptorSets.find(hash);
		if (it != m_DescriptorSets.end())
			return { true, it->second };

		if (m_Handle == VK_NULL_HANDLE)
		{
			m_Handle = Get();
			m_UsedPools.push_back(m_Handle);
		}
		
		VkDescriptorSet descriptor;

		VkDescriptorSetAllocateInfo info = {};
		info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		info.pSetLayouts = &layout;
		info.descriptorPool = m_Handle;
		info.descriptorSetCount = 1;

		VkResult result = vkAllocateDescriptorSets(m_Device->Get(), &info, &descriptor);
		switch (result)
		{
		case VK_SUCCESS:
			break;
		case VK_ERROR_FRAGMENTED_POOL:
		case VK_ERROR_OUT_OF_POOL_MEMORY:
			m_Handle = Get();
			m_UsedPools.push_back(m_Handle);
			vkAllocateDescriptorSets(m_Device->Get(), &info, &descriptor);
			break;
		default:
			throw std::runtime_error("Failed to allocate descriptor set");
			return { false, VK_NULL_HANDLE };
			break;
		}

		m_DescriptorSets.emplace(hash, descriptor);

		return { false, m_DescriptorSets[hash] };
	}

	VkDescriptorPool DescriptorSetPoolVK::Get()
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

	void DescriptorSetPoolVK::Reset()
	{
		for (auto pool : m_UsedPools)
			vkResetDescriptorPool(m_Device->Get(), pool, 0);

		m_FreePools = m_UsedPools;
		m_UsedPools.clear();
		m_Handle = VK_NULL_HANDLE;
	}

	VkDescriptorPool DescriptorSetPoolVK::CreatePool()
	{
		std::vector<VkDescriptorPoolSize> sizes;
		sizes.reserve(m_DescSizes.sizes.size());
		for (auto& size : m_DescSizes.sizes)
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
		for (auto& [key, layout] : m_Layout)
			vkDestroyDescriptorSetLayout(m_Device->Get(), layout, nullptr);
	}

	VkDescriptorSetLayout DescriptorSetLayoutCacheVK::GetLayout(const std::vector<ShaderResourceBinding> layout)
	{
		DSLKey key(layout);

		auto it = m_Layout.find(key);
		if (it == m_Layout.end())
		{
			VkDescriptorSetLayout descLayout;
			CreateNewLayout(key, &descLayout);
			it = m_Layout.insert(std::make_pair(key, std::move(descLayout))).first;
		}

		return it->second;
	}

	void DescriptorSetLayoutCacheVK::CreateNewLayout(const DSLKey& layout, VkDescriptorSetLayout* descLayout)
	{
		std::vector<VkDescriptorSetLayoutBinding> bindings;
		VkDescriptorSetLayoutCreateInfo createInfo = {};
		createInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;

		for (auto& resource : layout.Resources) {
			VkDescriptorType type;
			switch (resource.Type)
			{
			case ResourceBindingType::ImageSampler:
				type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
				break;
			case ResourceBindingType::ImageStorage:
				type = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
				break;
			case ResourceBindingType::UniformBuffer:
				type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
				break;
			case ResourceBindingType::StorageBuffer:
				type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
				break;
			case ResourceBindingType::InputAttachment:
				type = VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT;
				break;
			case ResourceBindingType::Sampler:
				type = VK_DESCRIPTOR_TYPE_SAMPLER;
				break;
			case ResourceBindingType::Image:
				type = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
				break;
			}
			bindings.push_back({ resource.Binding, type, 1, (VkShaderStageFlags)resource.Stages, 0 });
		}

		createInfo.bindingCount = static_cast<uint32_t>(bindings.size());
		createInfo.pBindings = bindings.data();

		vkCreateDescriptorSetLayout(m_Device->Get(), &createInfo, nullptr, descLayout);
	}

	DSLKey::DSLKey(std::vector<ShaderResourceBinding> resources)
	{
		Resources = resources;
	}

	bool DSLKey::operator==(const DSLKey& rhs) const
	{
		if (GetHash() != rhs.GetHash() || 
			Resources.size() != rhs.Resources.size())
			return false;
		return true;
	}

	size_t DSLKey::GetHash() const
	{
		if (Hash == 0)
		{
			for (auto& resource : Resources) {
				hash_combine(Hash, resource.Stages);
				hash_combine(Hash, resource.Type);
				hash_combine(Hash, resource.Mode);
				hash_combine(Hash, resource.Set);
				hash_combine(Hash, resource.Binding);
				hash_combine(Hash, resource.Location);
				hash_combine(Hash, resource.InputAttachIndex);
				hash_combine(Hash, resource.VecSize);
				hash_combine(Hash, resource.Columns);
				hash_combine(Hash, resource.ArraySize);
				hash_combine(Hash, resource.Offset);
				hash_combine(Hash, resource.Size);
				hash_combine(Hash, resource.ConstantId);
				hash_combine(Hash, resource.Qualifiers);
				hash_combine(Hash, resource.Name);
			}
		}
		return Hash;
	}
}
