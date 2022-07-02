#pragma once
#include "Buffer.h"

#include <vulkan/vulkan.h>

namespace Vulkan 
{
	class GraphicsDeviceVK;
	class BufferVK : public Buffer
	{
	public:
		BufferVK(GraphicsDeviceVK* device, const BufferDesc& desc, const void* data = nullptr);
		BufferVK(GraphicsDeviceVK* device, const BufferDesc& desc, VkBuffer buffer);

		~BufferVK();

		virtual void MapMemory(uint64_t offset, uint64_t size, void** data);
		virtual void FlushMemory();
		virtual void Expand(uint64_t size);

		VkBuffer Get() { return m_Buffer; }
		VkDeviceMemory GetMemory() { return m_Memory; }

	private:
		VkBuffer CreateBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags props, VkDeviceMemory& bufferMemory);
		void CopyFrom(VkBuffer src, VkDeviceSize size);
		uint32_t FindMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties);

	private:
		GraphicsDeviceVK* m_Device;

		VkBuffer m_Buffer;
		VkDeviceMemory m_Memory;

		VkDeviceSize m_Alignment = 256;

		void* m_StageData = nullptr;
	};
}
