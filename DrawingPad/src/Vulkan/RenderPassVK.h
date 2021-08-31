#pragma once
#include "RenderPass.h"
#include "UtilsVK.h"

#include <vulkan/vulkan.h>

namespace VkAPI {
	class GraphicsDeviceVK;
	class RenderPassVK : public RenderPass
	{
	public:
		RenderPassVK(GraphicsDevice* device, const RenderPassDesc& desc);

		~RenderPassVK();

		VkRenderPass GetRenderPass() const { return m_Pass; }

	private:
		inline static const VkAttachmentReference* ConvertAttachRefs (uint32_t refCount, const AttachmentReference* srcRefs, std::vector<VkAttachmentReference>* attach, uint32_t* counter)
		{
			VkAttachmentReference* curr = &(*attach)[*counter];
			for (uint32_t i = 0; i < refCount; i++, (*counter)++)
			{
				(*attach)[*counter].attachment = srcRefs[i].Attachment;
				(*attach)[*counter].layout = UtilsVK::Convert(srcRefs[i].Layout);
			}
			return curr;
		};

	private:
		VkRenderPass m_Pass;
	};
}
