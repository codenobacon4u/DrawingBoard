#pragma once
#include "Buffer.h"

#include <vulkan/vulkan.h>
#include <vma_mem_alloc.h>

namespace Vulkan 
{
	class GraphicsDeviceVK;
	class BufferVK : public Buffer
	{
	public:
		BufferVK(GraphicsDeviceVK* device, const BufferDesc& desc, uint8_t* data = nullptr);
		BufferVK(GraphicsDeviceVK* device, const BufferDesc& desc, VkBuffer buffer);

		~BufferVK();

		virtual void* MapMemory();
		virtual void UnmapMemory();
		virtual void FlushMemory();

		virtual void Update(uint64_t offset, uint64_t size, const void* data);

		VkBuffer Get() { return m_Buffer; }
		VkDeviceMemory GetMemory() { return m_Memory; }

	private:
		VkBuffer CreateBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags props, VkDeviceMemory& bufferMemory);
		void CopyFrom(VkBuffer src, VkDeviceSize size);
		uint32_t FindMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties);

	private:
		GraphicsDeviceVK* m_Device;

		VkBuffer m_Buffer = VK_NULL_HANDLE;
		VmaAllocation m_Alloc = VK_NULL_HANDLE;
		VkDeviceMemory m_Memory = VK_NULL_HANDLE;

		bool m_Mapped = false;
		bool m_Persist = false;

		void* m_StageData = nullptr;
	};
}
