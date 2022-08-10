#include "dppch.h"
#include "BufferVK.h"

#include "CommandBufferVK.h"
#include "GraphicsDeviceVK.h"

namespace DrawingPad
{
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
			vmaCreateBuffer(m_Device->GetMemoryAllocator(), &createInfo, &memInfo, &m_Handle, &m_Alloc, &allocInfo);

			m_Persist = memInfo.flags & VMA_ALLOCATION_CREATE_MAPPED_BIT;

			m_Memory = allocInfo.deviceMemory;

			if (m_Persist) {
				m_Data = allocInfo.pMappedData;
			}

			if (bufData != nullptr)
				Update(0, m_Desc.Size, bufData);
		}

		BufferVK::~BufferVK()
		{
			UnmapMemory();
			vmaDestroyBuffer(m_Device->GetMemoryAllocator(), m_Handle, m_Alloc);
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
			}
			else {
				MapMemory();
				std::copy((uint8_t*)data, (uint8_t*)data + size, (uint8_t*)m_Data + offset);
				FlushMemory();
				UnmapMemory();
			}
		}
	}
}
