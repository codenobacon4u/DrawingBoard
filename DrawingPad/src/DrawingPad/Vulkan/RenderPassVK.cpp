#include "dppch.h"
#include "RenderPassVK.h"
#include "GraphicsDeviceVK.h"

#include <array>

namespace DrawingPad
{
	namespace Vulkan
	{
		RenderPassVK::RenderPassVK(GraphicsDevice* device, const RenderPassDesc& desc)
			: RenderPass(device, desc)
		{
			VkRenderPassCreateInfo createInfo = {};
			createInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;

			std::vector<VkAttachmentDescription> attachments = {};
			for (auto& attachment : m_Desc.Attachments) {
				VkAttachmentDescription desc = {};
				desc.flags = 0;
				desc.format = UtilsVK::TextureFormatToVk(attachment.Format);
				desc.samples = static_cast<VkSampleCountFlagBits>(attachment.Samples);
				desc.loadOp = UtilsVK::LoadOpToVk(attachment.LoadOp);
				desc.storeOp = UtilsVK::StoreOpToVk(attachment.StoreOp);
				desc.stencilLoadOp = UtilsVK::LoadOpToVk(attachment.StencilLoadOp);
				desc.stencilStoreOp = UtilsVK::StoreOpToVk(attachment.StencilStoreOp);
				desc.initialLayout = UtilsVK::ImageLayoutToVk(attachment.InitialLayout);
				desc.finalLayout = UtilsVK::ImageLayoutToVk(attachment.FinalLayout);
				attachments.push_back(desc);
			}
			createInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
			createInfo.pAttachments = attachments.data();

			std::vector<VkSubpassDescription> subpasses = {};
			std::vector<std::vector<VkAttachmentReference>> inputs(m_Desc.Subpasses.size());
			std::vector<std::vector<VkAttachmentReference>> colors(m_Desc.Subpasses.size());
			std::vector<std::vector<VkAttachmentReference>> resolves(m_Desc.Subpasses.size());
			uint32_t i = 0;
			for (auto subpass : m_Desc.Subpasses) {
				VkSubpassDescription desc = {};
				desc.flags = 0;
				desc.pipelineBindPoint = static_cast<VkPipelineBindPoint>(subpass.BindPoint);
				desc.inputAttachmentCount = static_cast<uint32_t>(subpass.InputAttachments.size());
				for (auto input : subpass.InputAttachments) {
					inputs[i].push_back({ input.Attachment, UtilsVK::ImageLayoutToVk(input.Layout) });
				}
				desc.pInputAttachments = inputs[i].data();
				desc.colorAttachmentCount = static_cast<uint32_t>(subpass.ColorAttachments.size());
				for (auto input : subpass.ColorAttachments) {
					colors[i].push_back({ input.Attachment, UtilsVK::ImageLayoutToVk(input.Layout) });
				}
				desc.pColorAttachments = colors[i].data();
				for (auto input : subpass.ResolveAttachments) {
					resolves[i].push_back({ input.Attachment, UtilsVK::ImageLayoutToVk(input.Layout) });
				}
				desc.pResolveAttachments = resolves[i].data();
				VkAttachmentReference depthAttachment = {};
				if (subpass.DepthStencilAttachment) {
					depthAttachment.attachment = subpass.DepthStencilAttachment->Attachment;
					depthAttachment.layout = UtilsVK::ImageLayoutToVk(subpass.DepthStencilAttachment->Layout);
					desc.pDepthStencilAttachment = &depthAttachment;
				}
				subpasses.push_back(desc);
				i++;
			}
			createInfo.subpassCount = static_cast<uint32_t>(subpasses.size());
			createInfo.pSubpasses = subpasses.data();

			std::vector<VkSubpassDependency> dependencies = {};
			for (auto& dependency : m_Desc.SubpassDependencies) {
				VkSubpassDependency desc = {};
				desc.srcSubpass = dependency.SrcSubpass;
				desc.dstSubpass = dependency.DstSubpass;
				desc.srcStageMask = dependency.SrcStage;
				desc.dstStageMask = dependency.DstStage;
				desc.srcAccessMask = dependency.SrcAccess;
				desc.dstAccessMask = dependency.DstAccess;
				dependencies.push_back(desc);
			}
			createInfo.dependencyCount = static_cast<uint32_t>(dependencies.size());
			createInfo.pDependencies = dependencies.data();

			if (vkCreateRenderPass(((GraphicsDeviceVK*)m_Device)->Get(), &createInfo, nullptr, &m_Handle) != VK_SUCCESS)
				throw std::runtime_error("Failed to create RenderPass");
		}

		RenderPassVK::~RenderPassVK()
		{
			vkDestroyRenderPass(((GraphicsDeviceVK*)m_Device)->Get(), m_Handle, nullptr);
		}
	}
}
