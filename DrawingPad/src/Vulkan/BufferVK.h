#pragma once
#include "Buffer.h"

#include <vulkan/vulkan.h>

namespace VkAPI 
{
	class GraphicsDeviceVK;
	class BufferVK : public Buffer
	{
	public:
		BufferVK(GraphicsDeviceVK* device, const BufferDesc& desc, void* data = nullptr);
		BufferVK(GraphicsDeviceVK* device, const BufferDesc& desc, VkBuffer buffer);

		~BufferVK();

		virtual void BeginStaging() override;
		virtual void EndStaging() override;

		VkBuffer Get() { return m_Buffer; }
		VkDeviceMemory GetMemory() { return m_Memory; }

	private:
		VkBuffer CreateBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags props, VkDeviceMemory& bufferMemory);
		//void CopyBuffer(VkBuffer src, VkBuffer dst, VkDeviceSize size);
		uint32_t FindMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties);

	private:
		GraphicsDeviceVK* m_Device;

		VkBuffer m_Buffer;
		VkDeviceMemory m_Memory;

		void* m_StageData = nullptr;
	};
}
