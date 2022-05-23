#include "pwpch.h"
#include "RenderPassPoolVK.h"

#include "GraphicsDeviceVK.h"
#include "UtilsVK.h"

#include <chrono>
#include <array>

namespace Vulkan 
{
	using FloatingPointMicroseconds = std::chrono::duration<double, std::micro>;
	bool RPKey::operator==(const RPKey& rhs) const
	{
		if (GetHash() != rhs.GetHash() ||
			NumColors != rhs.NumColors ||
			SampleCount != rhs.SampleCount ||
			DepthFormat != rhs.DepthFormat || ClearEnable != rhs.ClearEnable)
			return false;
		for (uint32_t i = 0; i < NumColors; i++)
			if (ColorFormats[i] != rhs.ColorFormats[i])
				return false;
		return true;
	}

	size_t RPKey::GetHash() const
	{
		if (Hash == 0)
		{
			Hasher h;
			h.AddHash(NumColors);
			h.AddHash(SampleCount);
			h.AddHash(DepthFormat);
			for (uint32_t i = 0; i < NumColors; i++)
				h.AddHash(ColorFormats[i]);
		}
		return Hash;
	}

	RenderPassPoolVK::~RenderPassPoolVK()
	{
		for (const auto& [key, pass] : m_Array)
			delete pass;
		for (const auto& [key, pass] : m_Map)
			delete pass;
	}

	RenderPassVK* RenderPassPoolVK::GetRenderPass(const RPKey& key)
    {
		auto it = m_Map.find(key);
		if (it == m_Map.end())
		{
			std::vector<RenderPassAttachmentDesc> attachments;
			std::vector<AttachmentReference> refs;
			uint32_t doff = 0;
			if (key.DepthFormat != TextureFormat::Unknown)
			{
				doff = 1;
				attachments.push_back({
					key.DepthFormat,
					key.SampleCount,
					AttachmentLoadOp::Clear,
					AttachmentStoreOp::DontCare,
					AttachmentLoadOp::DontCare,
					AttachmentStoreOp::DontCare,
					ImageLayout::Undefined,
					ImageLayout::DepthStencilAttachOptimal
				});

				refs.push_back({ 0, ImageLayout::DepthStencilAttachOptimal });
			}

			for (uint32_t i = 0; i < key.NumColors; i++)
			{
				attachments.push_back({ 
					key.ColorFormats[i],
					key.SampleCount,
					key.ClearEnable ? AttachmentLoadOp::Clear : AttachmentLoadOp::DontCare, 
					AttachmentStoreOp::Store, 
					AttachmentLoadOp::Discard, 
					AttachmentStoreOp::Discard, 
					ImageLayout::Undefined, 
					ImageLayout::PresentSrcKHR 
				});

				refs.push_back({ i + doff, ImageLayout::ColorAttachOptimal });
			}

			SubpassDesc subpass = {};
			subpass.InputAttachmentCount = 0;
			subpass.InputAttachments = nullptr;
			subpass.ColorAttachmentCount = static_cast<uint32_t>(refs.size() - doff);
			subpass.ColorAttachments = &refs[doff];
			subpass.ResolveAttachments = nullptr;
			subpass.DepthStencilAttachment = key.DepthFormat != TextureFormat::Unknown ? &refs.front() : nullptr;
			subpass.PreserveAttachmentCount = 0;
			subpass.PreserveAttachments = nullptr;

			DependencyDesc dependency = {};
			dependency.SrcSubpass = VK_SUBPASS_EXTERNAL;
			dependency.DstSubpass = 0;
			dependency.SrcStage = (PipelineStage)(PipelineStage::ColorAttachOutput | PipelineStage::EarlyFragTests);
			dependency.SrcAccess = SubpassAccess::NA;
			dependency.DstStage = (PipelineStage)(PipelineStage::ColorAttachOutput | PipelineStage::EarlyFragTests);
			dependency.DstAccess = (SubpassAccess)(SubpassAccess::ColorAttachWrite | SubpassAccess::DepthStencilAttachWrite);


			RenderPassDesc desc;
			desc.AttachmentCount = static_cast<uint32_t>(attachments.size());
			desc.Attachments = attachments.data();
			desc.SubpassCount = 1;
			desc.Subpasses = &subpass;
			desc.DependencyCount = 1;
			desc.Dependencies = &dependency;

			RenderPassVK* rp = m_Device.CreateRenderPass(desc);
			it = m_Map.emplace(key, std::move(rp)).first;
		}
		return it->second;
	}
}
