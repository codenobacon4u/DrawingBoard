#include "pwpch.h"
#include "BufferVK.h"

#include "CommandBufferVK.h"
#include "GraphicsDeviceVK.h"

namespace Vulkan 
{
	BufferVK::BufferVK(GraphicsDeviceVK* device, const BufferDesc& desc, uint8_t* bufData)
		: Buffer(desc, bufData), m_Device(device)
	{
		VkBufferCreateInfo createInfo = {};
		VmaAllocationCreateInfo memInfo = {};
		memInfo.usage = VMA_MEMORY_USAGE_AUTO;

		switch (m_Desc.BindFlags)
		{
		case BufferBindFlags::Vertex:
			createInfo.usage |= VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
			memInfo.flags |= VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT;
			break;
		case BufferBindFlags::Index:
			createInfo.usage |= VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
			memInfo.flags |= VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT;
			break;
		case BufferBindFlags::Staging:
			memInfo.flags |= VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT;
			createInfo.usage |= VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
			break;
		case BufferBindFlags::Uniform:
			createInfo.usage |= VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
			memInfo.flags |= VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT;
			break;
		case BufferBindFlags::ShaderResource:
			if (m_Desc.Mode == BufferModeFlags::Formatted)
				createInfo.usage |= VK_BUFFER_USAGE_UNIFORM_TEXEL_BUFFER_BIT;
			else
				createInfo.usage |= VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
			break;
		case BufferBindFlags::Unordered:
			if (m_Desc.Mode == BufferModeFlags::Formatted)
				createInfo.usage |= VK_BUFFER_USAGE_STORAGE_TEXEL_BUFFER_BIT;
			else
				createInfo.usage |= VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
			break;
		case BufferBindFlags::IndirectDraw:
			createInfo.usage |= VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT;
			break;
		case BufferBindFlags::RayTracing:
			createInfo.usage |= VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
			createInfo.usage |= VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;
			createInfo.usage |= VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR;
			createInfo.usage |= VK_BUFFER_USAGE_SHADER_BINDING_TABLE_BIT_KHR;
			break;
		default:
			break;
		}

		createInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		createInfo.size = m_Desc.Size;
		createInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

		VmaAllocationInfo allocInfo = {};
		vmaCreateBuffer(m_Device->GetMemoryAllocator(), &createInfo, &memInfo, &m_Buffer, &m_Alloc, &allocInfo);

		m_Persist = memInfo.flags & VMA_ALLOCATION_CREATE_MAPPED_BIT;

		m_Memory = allocInfo.deviceMemory;

		if (m_Persist) {
			m_Data = allocInfo.pMappedData;
		}

		if (bufData != nullptr)
			Update(0, m_Desc.Size, bufData);
	}

	BufferVK::BufferVK(GraphicsDeviceVK* device, const BufferDesc& desc, VkBuffer buffer)
		: Buffer(desc), m_Device(device), m_Buffer(buffer)
	{
	}

	BufferVK::~BufferVK()
	{
		if (m_Buffer != VK_NULL_HANDLE && m_Alloc != VK_NULL_HANDLE)
		{
			UnmapMemory();
			vmaDestroyBuffer(m_Device->GetMemoryAllocator(), m_Buffer, m_Alloc);
		}
	}

	void* BufferVK::MapMemory()
	{
		if (!m_Mapped && !m_Data)
		{
			vmaMapMemory(m_Device->GetMemoryAllocator(), m_Alloc, &m_Data);
			m_Mapped = true;
		}
		return m_Data;
	}

	void BufferVK::UnmapMemory()
	{
		if (m_Mapped)
		{
			vmaUnmapMemory(m_Device->GetMemoryAllocator(), m_Alloc);
			m_Data = nullptr;
			m_Mapped = false;
		}
	}

	void BufferVK::FlushMemory()
	{
		vmaFlushAllocation(m_Device->GetMemoryAllocator(), m_Alloc, 0, m_Desc.Size);
	}

	void BufferVK::Update(uint64_t offset, uint64_t size, const void* data)
	{
		if (m_Persist) {
			std::copy((uint8_t*)data, (uint8_t*)data + size, (uint8_t*)m_Data + offset);
			FlushMemory();
		} else {
			MapMemory();
			std::copy((uint8_t*)data, (uint8_t*)data + size, (uint8_t*)m_Data + offset);
			FlushMemory();
			UnmapMemory();
		}
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

		return VK_NULL_HANDLE;
	}

	void BufferVK::CopyFrom(VkBuffer src, VkDeviceSize size)
	{
		VkQueue graphics = m_Device->GetGraphicsQueue();
		VkCommandBuffer cmd = static_cast<CommandBufferVK*>(m_Device->GetTempCommandPool().RequestCommandBuffer())->Get();
		VkBufferCopy copy{ 0, 0, size };
		VkCommandBufferBeginInfo info = {};
		info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
		vkBeginCommandBuffer(cmd, &info);
		vkCmdCopyBuffer(cmd, src, m_Buffer, 1, &copy);
		vkEndCommandBuffer(cmd);
		VkSubmitInfo submit = {};
		submit.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		submit.commandBufferCount = 1;
		submit.pCommandBuffers = &cmd;
		vkQueueSubmit(graphics, 1, &submit, VK_NULL_HANDLE);
		vkQueueWaitIdle(graphics);
		//m_Device->GetTempCommandPool().ReturnBuffer(std::move(cmd));
	}
}
