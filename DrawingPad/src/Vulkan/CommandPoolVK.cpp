#include "dppch.h"
#include "CommandPoolVK.h"

#include "GraphicsDeviceVK.h"
#include "TextureVK.h"

namespace Vulkan
{
	CommandPoolVK::CommandPoolVK(GraphicsDeviceVK* device, uint32_t queueFamilyIndex, VkCommandPoolCreateFlags flags)
		: CommandPool(device), m_Device(device)
	{
		VkCommandPool pool = VK_NULL_HANDLE;
		VkCommandPoolCreateInfo createInfo = {};
		createInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
		createInfo.flags = flags;
		createInfo.queueFamilyIndex = queueFamilyIndex;
		vkCreateCommandPool(device->Get(), &createInfo, nullptr, &m_Pool);
	}

	CommandPoolVK::~CommandPoolVK()
	{
		auto device = static_cast<GraphicsDeviceVK*>(m_Device);
		m_PrimaryBuffers.clear();
		m_SecondaryBuffers.clear();
		vkDestroyCommandPool(device->Get(), m_Pool, nullptr);
	}

	CommandBuffer* CommandPoolVK::RequestCommandBuffer(CommandBufferType type)
	{
		if (type == CommandBufferType::Primary) {
			if (m_ActivePrimaryCount < m_PrimaryBuffers.size())
			{
				return m_PrimaryBuffers[m_ActivePrimaryCount++];
			}
			m_ActivePrimaryCount++;
			m_PrimaryBuffers.push_back(DBG_NEW CommandBufferVK(m_Device, m_Pool, VK_COMMAND_BUFFER_LEVEL_PRIMARY));
			return m_PrimaryBuffers.back();
		}
		else {
			if (m_ActiveSecondaryCount < m_SecondaryBuffers.size())
			{
				return m_SecondaryBuffers[m_ActiveSecondaryCount++];
			}
			m_ActiveSecondaryCount++;
			m_SecondaryBuffers.push_back(DBG_NEW CommandBufferVK(m_Device, m_Pool, VK_COMMAND_BUFFER_LEVEL_SECONDARY));
			return m_SecondaryBuffers.back();
		}
	}

	void CommandPoolVK::Reset()
	{
		vkResetCommandPool(m_Device->Get(), m_Pool, 0);
		m_ActivePrimaryCount = 0;
		m_ActiveSecondaryCount = 0;
	}
}
