#pragma once
#include "CommandPool.h"

#include <deque>
#include <mutex>

#include <vulkan/vulkan.h>

#include "CommandBufferVK.h"

namespace Vulkan
{
	class GraphicsDeviceVK;
    class CommandPoolVK : public CommandPool
    {
	public:
		CommandPoolVK(GraphicsDeviceVK* device, uint32_t queueFamilyIndex, VkCommandPoolCreateFlags flags);

		CommandPoolVK(const CommandPoolVK&) = delete;
		CommandPoolVK(CommandPoolVK&&) = delete;

		~CommandPoolVK();

		virtual CommandBuffer* RequestCommandBuffer(CommandBufferType type = CommandBufferType::Primary) override;
		virtual void Reset() override;
	private:
		GraphicsDeviceVK* m_Device;
		VkCommandPool m_Pool;
		std::vector<CommandBufferVK*> m_PrimaryBuffers = {};
		std::vector<CommandBufferVK*> m_SecondaryBuffers = {};
		uint32_t m_ActivePrimaryCount = 0;
		uint32_t m_ActiveSecondaryCount = 0;
    };
}
