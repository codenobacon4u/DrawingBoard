#pragma once

#include <deque>
#include <mutex>

#include <vulkan/vulkan.h>

namespace Vulkan
{
	class GraphicsDeviceVK;
	class FramebufferVK;
    class CommandPoolVK
    {
	public:
		CommandPoolVK(GraphicsDeviceVK* device, uint32_t queueFamilyIndex, VkCommandPoolCreateFlags flags);

		CommandPoolVK(const CommandPoolVK&) = delete;
		CommandPoolVK(CommandPoolVK&&) = delete;

		~CommandPoolVK();

		void Reset();

		VkCommandBuffer GetBuffer();
		void ReturnBuffer(VkCommandBuffer&& buffer);
	private:
		GraphicsDeviceVK* m_Device;
		VkCommandPool m_CmdPool;
		std::mutex m_Mutex;
		std::deque<VkCommandBuffer> m_Buffers;
    };
}
