#pragma once
#include "RenderPass.h"
#include "UtilsVK.h"

#include <vulkan/vulkan.h>

namespace Vulkan {
	class GraphicsDeviceVK;
	class RenderPassVK : public RenderPass
	{
	public:
		RenderPassVK(GraphicsDevice* device, const RenderPassDesc& desc);
		~RenderPassVK();

		VkRenderPass Get() const { return m_Pass; }

	private:
		inline static std::vector<VkAttachmentReference> ConvertAttachments(const std::vector<AttachmentReference>& srcRefs) {
			std::vector<VkAttachmentReference> res = {};
			for (auto input : srcRefs) {
				res.push_back({ input.Attachment, UtilsVK::Convert(input.Layout) });
			}
			return res;
		}

	private:
		VkRenderPass m_Pass = VK_NULL_HANDLE;
	};
}
