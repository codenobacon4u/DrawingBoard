#include "pwpch.h"
#include "BufferVK.h"

#include "CommandBufferVK.h"
#include "GraphicsDeviceVK.h"

namespace Vulkan 
{
	BufferVK::BufferVK(GraphicsDeviceVK* device, const BufferDesc& desc, const void* bufData)
		: Buffer(desc, bufData), m_Device(device)
	{
		VkMemoryPropertyFlags properties = 0;
		VkBufferUsageFlags usage = 0;
		bool staging = false;
		switch (m_Desc.BindFlags)
		{
		case BufferBindFlags::Vertex:
			usage |= VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
			properties |= VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT;
			staging = true;
			break;
		case BufferBindFlags::Index:
			usage |= VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
			properties |= VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT;
			staging = true;
			break;
		case BufferBindFlags::Staging:
			usage |= VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
			properties |= VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
			break;
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

		if (staging && bufData != nullptr)
		{
			VkDeviceMemory stagingMem;
			VkBuffer stagingBuff = CreateBuffer(m_Desc.Size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingMem);

			void* data;
			vkMapMemory(m_Device->Get(), stagingMem, 0, m_Desc.Size, 0, &data);
			memcpy(data, bufData, (size_t)m_Desc.Size);
			vkUnmapMemory(m_Device->Get(), stagingMem);

			CopyFrom(stagingBuff, m_Desc.Size);

			vkDestroyBuffer(m_Device->Get(), stagingBuff, nullptr);
			vkFreeMemory(m_Device->Get(), stagingMem, nullptr);
		}
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

	void BufferVK::MapMemory(uint64_t offset, uint64_t size, void** data)
	{
		if (vkMapMemory(m_Device->Get(), m_Memory, offset, size, 0, data) != VK_SUCCESS)
			UtilsVK::Log("validation_layers.log", "Failed to Map Memory!");
	}

	void BufferVK::FlushMemory()
	{
		//VkMappedMemoryRange range = {};
		//range.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
		//range.memory = m_Memory;
		//range.size = rangeSize;
		//vkFlushMappedMemoryRanges(m_Device->Get(), 1, &range);
		vkUnmapMemory(m_Device->Get(), m_Memory);
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
		VkDeviceSize alignedSize = ((size - 1) / m_Alignment + 1) * m_Alignment;

		VkBufferCreateInfo createInfo = {};
		createInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		createInfo.size = size;
		createInfo.usage = usage;
		createInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

		VkBuffer buffer;
		vkCreateBuffer(m_Device->Get(), &createInfo, nullptr, &buffer);

		VkMemoryRequirements memReq;
		vkGetBufferMemoryRequirements(m_Device->Get(), buffer, &memReq);
		m_Alignment = (m_Alignment > memReq.alignment) ? m_Alignment : memReq.alignment;
		//UtilsVK::Log("debug.log", std::to_string((uint64_t)m_Device->GetPhysicalLimits().minUniformBufferOffsetAlignment));
		VkMemoryAllocateInfo allocInfo = {};
		allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		allocInfo.allocationSize = memReq.size;
		allocInfo.memoryTypeIndex = FindMemoryType(memReq.memoryTypeBits, props);

		vkAllocateMemory(m_Device->Get(), &allocInfo, nullptr, &bufferMemory);
		vkBindBufferMemory(m_Device->Get(), buffer, bufferMemory, 0);
		//m_Desc.Size = memReq.size;
		return buffer;
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
