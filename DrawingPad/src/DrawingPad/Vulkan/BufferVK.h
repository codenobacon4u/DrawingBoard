#pragma once
#include "DrawingPad/Buffer.h"

#include "StructsVK.h"

#include <vulkan/vulkan.h>
#include <vma_mem_alloc.h>

namespace DrawingPad
{
	namespace Vulkan
	{
		class GraphicsDeviceVK;
		class BufferVK : public Buffer
		{
		public:
			BufferVK(GraphicsDeviceVK* device, const BufferDesc& desc, uint8_t* data = nullptr);

			~BufferVK();

			virtual void* MapMemory();
			virtual void UnmapMemory();
			virtual void FlushMemory();

			virtual void Update(uint64_t offset, uint64_t size, const void* data);

			VkBuffer Get() { return m_Handle; }
			VkDeviceMemory GetMemory() { return m_Memory; }

		private:
			GraphicsDeviceVK* m_Device;

			VkBuffer m_Handle = VK_NULL_HANDLE;
			VmaAllocation m_Alloc = VK_NULL_HANDLE;
			VkDeviceMemory m_Memory = VK_NULL_HANDLE;

			bool m_Mapped = false;
			bool m_Persist = false;
		};
	}
}
