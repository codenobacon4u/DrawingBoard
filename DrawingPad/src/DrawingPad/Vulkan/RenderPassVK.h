#pragma once
#include "DrawingPad/RenderPass.h"

#include "UtilsVK.h"

#include <vulkan/vulkan.h>

namespace DrawingPad
{
	namespace Vulkan
	{
		class GraphicsDeviceVK;
		class RenderPassVK : public RenderPass
		{
		public:
			RenderPassVK(GraphicsDevice* device, const RenderPassDesc& desc);
			~RenderPassVK();

			VkRenderPass Get() const { return m_Handle; }

		private:
			inline static std::vector<VkAttachmentReference> ConvertAttachments(const std::vector<AttachmentReference>& srcRefs) {
				std::vector<VkAttachmentReference> res = {};
				for (auto& input : srcRefs) {
					res.push_back({ input.Attachment, UtilsVK::ImageLayoutToVk(input.Layout) });
				}
				return res;
			}

		private:
			VkRenderPass m_Handle = VK_NULL_HANDLE;
		};
	}
}
