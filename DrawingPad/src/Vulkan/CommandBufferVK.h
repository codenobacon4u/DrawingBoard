#pragma once

#include <vulkan/vulkan.h>

namespace Vulkan
{
	class CommandBufferVK
	{
	public:
		CommandBufferVK()
		{}

		CommandBufferVK(const CommandBufferVK&) = delete;
		CommandBufferVK(CommandBufferVK&&) = delete;
		CommandBufferVK& operator=(const CommandBufferVK&) = delete;
		CommandBufferVK& operator=(CommandBufferVK&&) = delete;

		inline void BeginRenderPass(VkRenderPass pass, VkFramebuffer buffer, uint32_t width, uint32_t height, uint32_t clearCount = 0, const VkClearValue* clearValues = nullptr) 
		{
			if (m_State.RenderPass != pass || m_State.Framebuffer != buffer)
			{

				VkRenderPassBeginInfo beginInfo = {};
				beginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
				beginInfo.clearValueCount = clearCount;
				beginInfo.pClearValues = clearValues;
				beginInfo.renderPass = pass;
				beginInfo.framebuffer = buffer;
				beginInfo.renderArea = { {0, 0}, {width, height} };
				vkCmdBeginRenderPass(m_CmdBuffer, &beginInfo, VK_SUBPASS_CONTENTS_INLINE);
				m_State.RenderPass = pass;
				m_State.Framebuffer = buffer;
				m_State.FramebufferWidth = width;
				m_State.FramebufferHeight = height;
			}
		}

		inline void EndRenderPass()
		{
			vkCmdEndRenderPass(m_CmdBuffer);
			m_State.RenderPass = VK_NULL_HANDLE;
			m_State.Framebuffer = VK_NULL_HANDLE;
			m_State.FramebufferWidth = 0;
			m_State.FramebufferHeight = 0;
		}

		inline void EndCommandBuffer()
		{
			vkEndCommandBuffer(m_CmdBuffer);
		}

		inline void Reset()
		{
			m_CmdBuffer = VK_NULL_HANDLE;
			m_State = State{};
		}

		inline void SetViewports(uint32_t first, uint32_t count, const VkViewport* viewports)
		{
			vkCmdSetViewport(m_CmdBuffer, first, count, viewports);
		}

		inline void SetScissors(uint32_t first, uint32_t count, const VkRect2D* scissors)
		{
			vkCmdSetScissor(m_CmdBuffer, first, count, scissors);
		}

		inline void SetBuffer(VkCommandBuffer buffer)
		{
			m_CmdBuffer = buffer;
		}
		VkCommandBuffer Get() const { return m_CmdBuffer; }

	public:
		typedef struct State
		{
			VkRenderPass RenderPass = VK_NULL_HANDLE;
			VkFramebuffer Framebuffer = VK_NULL_HANDLE;
			VkPipeline GraphicsPipeline = VK_NULL_HANDLE;
			VkPipeline ComputePipeline = VK_NULL_HANDLE;
			VkPipeline RaytracingPipeline = VK_NULL_HANDLE;
			VkBuffer IndexBuffer = VK_NULL_HANDLE;
			VkDeviceSize IndexBufferOffset = 0;
			uint32_t FramebufferWidth = 0;
			uint32_t FramebufferHeight = 0;
		} State;

		const State GetState()& { return m_State; }

	private:
		State m_State;
		VkCommandBuffer m_CmdBuffer = VK_NULL_HANDLE;
	};
}
