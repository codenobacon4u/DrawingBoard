#include "pwpch.h"
#include "BufferVK.h"

#include "GraphicsDeviceVK.h"

namespace VkAPI 
{
	BufferVK::BufferVK(GraphicsDeviceVK* device, const BufferDesc& desc, void* bufData)
		: Buffer(desc, bufData), m_Device(device)
	{
		VkMemoryPropertyFlags properties = 0;
		VkBufferUsageFlags usage = 0;
		switch (m_Desc.BindFlags)
		{
		case BufferBindFlags::Vertex:
			usage |= VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
			properties |= VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
			break;
		case BufferBindFlags::Index:
			usage |= VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
			properties |= VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
			break;
		case BufferBindFlags::Staging:
			usage |= VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
			properties |= VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
		case BufferBindFlags::Uniform:
			usage |= VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
			properties |= VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
			break;
		case BufferBindFlags::ShaderResource:
			if (m_Desc.Mode == BufferModeFlags::Formatted)
				usage |= VK_BUFFER_USAGE_UNIFORM_TEXEL_BUFFER_BIT;
			else
				usage |= VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
			break;
		case BufferBindFlags::Unordered:
			if (m_Desc.Mode == BufferModeFlags::Formatted)
				usage |= VK_BUFFER_USAGE_STORAGE_TEXEL_BUFFER_BIT;
			else
				usage |= VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
			break;
		case BufferBindFlags::IndirectDraw:
			usage |= VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT;
			break;
		case BufferBindFlags::RayTracing:
			usage |= VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
			usage |= VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;
			usage |= VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR;
			usage |= VK_BUFFER_USAGE_SHADER_BINDING_TABLE_BIT_KHR;
			break;
		default:
			break;
		}

		m_Buffer = CreateBuffer(m_Desc.Size, usage, properties, m_Memory);
	}

	BufferVK::BufferVK(GraphicsDeviceVK* device, const BufferDesc& desc, VkBuffer buffer)
		: Buffer(desc), m_Device(device)
	{
	}

	BufferVK::~BufferVK()
	{
		vkDestroyBuffer(m_Device->Get(), m_Buffer, nullptr);
		vkFreeMemory(m_Device->Get(), m_Memory, nullptr);
	}

	void BufferVK::BeginStaging()
	{
		void* data;
		vkMapMemory(m_Device->Get(), m_Memory, 0, m_Desc.Size, 0, &data);
		memcpy(data, m_Data, m_Desc.Size);
		vkUnmapMemory(m_Device->Get(), m_Memory);
	}

	void BufferVK::EndStaging()
	{
	}

	uint32_t BufferVK::FindMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties) {
		VkPhysicalDeviceMemoryProperties memProperties;
		vkGetPhysicalDeviceMemoryProperties(m_Device->GetPhysical(), &memProperties);

		for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
			if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
				return i;
			}
		}

		throw std::runtime_error("Failed to find suitable memory type!");
	}

	VkBuffer BufferVK::CreateBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags props, VkDeviceMemory& bufferMemory)
	{
		VkBufferCreateInfo createInfo = {};
		createInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		createInfo.size = size;
		createInfo.usage = usage;
		createInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

		VkBuffer buffer;
		vkCreateBuffer(m_Device->Get(), &createInfo, nullptr, &buffer);

		VkMemoryRequirements memReq;
		vkGetBufferMemoryRequirements(m_Device->Get(), buffer, &memReq);

		VkMemoryAllocateInfo allocInfo = {};
		allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		allocInfo.allocationSize = memReq.size;
		allocInfo.memoryTypeIndex = FindMemoryType(memReq.memoryTypeBits, props);

		vkAllocateMemory(m_Device->Get(), &allocInfo, nullptr, &bufferMemory);
		vkBindBufferMemory(m_Device->Get(), buffer, bufferMemory, 0);
		return buffer;
	}
}