#pragma once
#include <vulkan/vulkan.h>

#include "DrawingPad/Texture.h"
#include "DrawingPad/Pipeline.h"
#include "DrawingPad/RenderPass.h"

namespace DrawingPad
{
	namespace Vulkan
	{
		class UtilsVK
		{
		public:
			static VkFormat TextureFormatToVk(TextureFormat format);
			static TextureFormat VkToTextureFormat(VkFormat format);
			static VkAttachmentLoadOp LoadOpToVk(AttachmentLoadOp format);
			static VkAttachmentStoreOp StoreOpToVk(AttachmentStoreOp format);
			static VkImageLayout ImageLayoutToVk(ImageLayout format);
			static SampleCount ToSampleCount(uint8_t samples);
			static VkFormat AttribFormatToVk(ElementDataType type, uint32_t num, bool normalized);
			static VkPipelineBindPoint PipelineBindPointToVk(PipelineBindPoint bindPoint);
			static void PrintDeviceProps(VkPhysicalDeviceProperties props);
		};
	}
}
