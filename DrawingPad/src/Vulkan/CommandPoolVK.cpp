#include "pwpch.h"
#include "CommandPoolVK.h"

#include "GraphicsDeviceVK.h"
#include "TextureVK.h"

namespace Vulkan
{
	CommandPoolVK::CommandPoolVK(GraphicsDeviceVK* device, uint32_t queueFamilyIndex, VkCommandPoolCreateFlags flags)
		: m_Device(device), m_CmdPool(VK_NULL_HANDLE)
	{
		VkCommandPoolCreateInfo createInfo = {};
		createInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
		createInfo.queueFamilyIndex = queueFamilyIndex;
		createInfo.flags = flags;
		vkCreateCommandPool(m_Device->Get(), &createInfo, nullptr, &m_CmdPool);

		for (uint32_t i = 0; i < 3; i++) {
			VkCommandBuffer buffer = VK_NULL_HANDLE;
			VkCommandBufferAllocateInfo allocInfo = {};
			allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
			allocInfo.pNext = nullptr;
			allocInfo.commandPool = m_CmdPool;
			allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
			allocInfo.commandBufferCount = 1;
			if (vkAllocateCommandBuffers(m_Device->Get(), &allocInfo, &buffer) != VK_SUCCESS)
				throw DBG_NEW std::runtime_error("Failed to create command buffer");
			m_Buffers.push_back(std::move(buffer));
		}
	}

	CommandPoolVK::~CommandPoolVK()
	{
		for (auto buff : m_Buffers)
			vkFreeCommandBuffers(m_Device->Get(), m_CmdPool, 1, &buff);
		vkDestroyCommandPool(m_Device->Get(), m_CmdPool, nullptr);
		m_CmdPool = VK_NULL_HANDLE;
	}

	void CommandPoolVK::Reset()
	{
		vkResetCommandPool(m_Device->Get(), m_CmdPool, 0);
	}

	VkCommandBuffer CommandPoolVK::GetBuffer()
	{
		VkCommandBuffer buffer = VK_NULL_HANDLE;

		{
			std::lock_guard<std::mutex> Lock{ m_Mutex };
			if (!m_Buffers.empty())
			{
				buffer = m_Buffers.front();
				m_Buffers.pop_front();
			}
		}

		if (buffer == VK_NULL_HANDLE)
		{
			VkCommandBufferAllocateInfo allocInfo = {};
			allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
			allocInfo.pNext = nullptr;
			allocInfo.commandPool = m_CmdPool;
			allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
			allocInfo.commandBufferCount = 1;
			if (vkAllocateCommandBuffers(m_Device->Get(), &allocInfo, &buffer) != VK_SUCCESS)
				throw DBG_NEW std::runtime_error("Failed to create command buffer");
		}
		VkCommandBufferBeginInfo beginInfo = {};
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		beginInfo.pNext = nullptr;
		beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
		vkBeginCommandBuffer(buffer, &beginInfo);
		
		return buffer;
	}

	void CommandPoolVK::ReturnBuffer(VkCommandBuffer&& buffer)
	{
		std::lock_guard<std::mutex> Lock{ m_Mutex };
		m_Buffers.emplace_back(buffer);
		buffer = VK_NULL_HANDLE;
	}
}