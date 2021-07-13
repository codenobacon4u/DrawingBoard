#include "pwpch.h"
#include "RenderPassVK.h"
#include "GraphicsDeviceVK.h"

#include <array>

namespace VkAPI {
	RenderPassVK::RenderPassVK(GraphicsDevice* device, const RenderPassDesc& desc)
		: RenderPass(device, desc)
	{
		VkRenderPassCreateInfo createInfo = {};
		createInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
		createInfo.flags = 0;
		
		std::vector<VkAttachmentDescription> attachments(desc.AttachmentCount);
		for (uint32_t i = 0; i < desc.AttachmentCount; i++)
		{
			const auto& attach = desc.Attachments[i];
			attachments[i].flags = 0;
			attachments[i].format = UtilsVK::Convert(attach.Format);
			attachments[i].samples = static_cast<VkSampleCountFlagBits>(attach.Samples);
			attachments[i].loadOp = UtilsVK::Convert(attach.LoadOp);
			attachments[i].storeOp = UtilsVK::Convert(attach.StoreOp);
			attachments[i].stencilLoadOp = UtilsVK::Convert(attach.StencilLoadOp);
			attachments[i].stencilStoreOp = UtilsVK::Convert(attach.StencilStoreOp);
			attachments[i].initialLayout = UtilsVK::Convert(attach.InitialLayout);
			attachments[i].finalLayout = UtilsVK::Convert(attach.FinalLayout);
		}
		createInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
		createInfo.pAttachments = attachments.data();

		uint32_t totalAttachments = 0;
		uint32_t currAttachment = 0;

		for (uint32_t i = 0; i < desc.SubpassCount; i++)
		{
			totalAttachments += desc.Subpasses[i].InputAttachmentCount + desc.Subpasses[i].ColorAttachmentCount;
			if (desc.Subpasses[i].ResolveAttachments)
				totalAttachments += desc.Subpasses[i].ColorAttachmentCount;
			if (desc.Subpasses[i].DepthStencilAttachment)
				totalAttachments++;
		}

		std::vector<VkAttachmentReference> attachs(totalAttachments);

		std::vector<VkSubpassDescription> subpasses(desc.SubpassCount);
		for (uint32_t i = 0; i < desc.SubpassCount; i++)
		{
			auto& subpass = desc.Subpasses[i];

			if (subpass.InputAttachmentCount > 0)
			{
				subpasses[i].inputAttachmentCount = subpass.InputAttachmentCount;
				subpasses[i].pInputAttachments = ConvertAttachRefs(subpass.InputAttachmentCount, subpass.InputAttachments, &attachs, &currAttachment);
			}

			if (subpass.ColorAttachmentCount > 0)
			{
				subpasses[i].colorAttachmentCount = subpass.ColorAttachmentCount;
				subpasses[i].pColorAttachments = ConvertAttachRefs(subpass.ColorAttachmentCount, subpass.ColorAttachments, &attachs, &currAttachment);
				
				if (subpass.ResolveAttachments != nullptr)
				{
					subpasses[i].pResolveAttachments = ConvertAttachRefs(subpass.ColorAttachmentCount, subpass.ResolveAttachments, &attachs, &currAttachment);
				}
			}

			if (subpass.PreserveAttachmentCount > 0)
			{
				subpasses[i].preserveAttachmentCount = subpass.PreserveAttachmentCount;
				subpasses[i].pPreserveAttachments = subpass.PreserveAttachments;
			}

			if (subpass.DepthStencilAttachment)
			{
				VkAttachmentReference depth = { subpass.DepthStencilAttachment->Attachment, UtilsVK::Convert(subpass.DepthStencilAttachment->Layout) };
				subpasses[i].pDepthStencilAttachment = &depth;
			}
		}
		createInfo.subpassCount = static_cast<uint32_t>(subpasses.size());
		createInfo.pSubpasses = subpasses.data();

		std::vector<VkSubpassDependency> dependencies(desc.DependencyCount);
		for (uint32_t i = 0; i < desc.DependencyCount; i++)
		{
			const auto& dep = desc.Dependencies[i];
			dependencies[i].srcSubpass = dep.SrcSubpass;
			dependencies[i].dstSubpass = dep.DstSubpass;
			dependencies[i].srcStageMask = dep.SrcStage;
			dependencies[i].dstStageMask = dep.DstStage;
			dependencies[i].srcAccessMask = dep.SrcAccess;
			dependencies[i].dstAccessMask = dep.DstAccess;
		}
		createInfo.dependencyCount = static_cast<uint32_t>(dependencies.size());
		createInfo.pDependencies = dependencies.data();

		vkCreateRenderPass(((GraphicsDeviceVK*)m_Device)->Get(), &createInfo, nullptr, &m_Pass);
	}

	RenderPassDesc RenderPassVK::GetDefaultDesc(uint32_t numTargets, const TextureFormat* colorFormats, TextureFormat depthFormat, SampleCount samples, std::array<RenderPassAttachmentDesc, 9> attachments, std::array<AttachmentReference, 9> refs, SubpassDesc subpass)
	//RenderPassDesc RenderPassVK::GetDefaultDesc(uint32_t numTargets, const TextureFormat* colorFormats, TextureFormat depthFormat, SampleCount samples)
	{
		RenderPassDesc desc;
		//std::array<RenderPassAttachmentDesc, 9> attachments;
		//std::array<AttachmentReference, 9> refs;
		//SubpassDesc subpass;

		uint32_t attachIdx = 0;
		AttachmentReference* depthAttachRef = nullptr;
		if (depthFormat != TextureFormat::Unknown)
		{
			auto& depth = attachments[0];
			depth.Format = depthFormat;
			depth.Samples = samples;
			depth.LoadOp = AttachmentLoadOp::Clear;
			depth.StoreOp = AttachmentStoreOp::Store;
			depth.StencilLoadOp = AttachmentLoadOp::Clear;
			depth.StencilStoreOp = AttachmentStoreOp::Store;
			depth.InitialLayout = ImageLayout::DepthAttachOptimal;
			depth.FinalLayout = ImageLayout::DepthAttachOptimal;
			depthAttachRef = &refs[0];
			depthAttachRef->Attachment = 0;
			depthAttachRef->Layout = ImageLayout::DepthAttachOptimal;

			attachIdx++;
		}

		AttachmentReference* colorAttachRefs = &refs[attachIdx];
		if (numTargets == 0)
			colorAttachRefs = nullptr;

		desc.AttachmentCount = numTargets + attachIdx;
		for (uint32_t i = 0; i < desc.AttachmentCount; i++, attachIdx++) {
			auto& color = attachments[attachIdx];
			color.Format = colorFormats[i];
			color.Samples = samples;
			color.LoadOp = AttachmentLoadOp::Clear;
			color.StoreOp = AttachmentStoreOp::Store;
			color.StencilLoadOp = AttachmentLoadOp::Discard;
			color.StencilStoreOp = AttachmentStoreOp::Discard;
			color.InitialLayout = ImageLayout::Undefined;
			color.FinalLayout = ImageLayout::PresentSrcKHR;
			auto& colorRef = refs[attachIdx];
			colorRef.Attachment = attachIdx;
			colorRef.Layout = ImageLayout::ColorAttachOptimal;
		}

		desc.Attachments = attachments.data();
		desc.SubpassCount = 1;
		desc.Subpasses = &subpass;
		desc.DependencyCount = 0;
		desc.Dependencies = nullptr;

		subpass.InputAttachmentCount = 0;
		subpass.InputAttachments = nullptr;
		subpass.ColorAttachmentCount = numTargets;
		subpass.ColorAttachments = colorAttachRefs;
		subpass.ResolveAttachments = nullptr;
		subpass.DepthStencilAttachment = depthAttachRef;
		subpass.PreserveAttachmentCount = 0;
		subpass.PreserveAttachments = nullptr;

		return desc;
	}
}