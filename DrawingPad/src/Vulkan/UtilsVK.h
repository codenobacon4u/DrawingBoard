#pragma once
#include <vulkan/vulkan.h>

#include "Texture.h"
#include "Pipeline.h"
#include "RenderPass.h"

namespace Vulkan
{
	class UtilsVK
	{
	public:
		static void Log(std::string path, std::string msg);
		static VkFormat Convert(TextureFormat format);
		static TextureFormat Convert(VkFormat format);
		static VkAttachmentLoadOp Convert(AttachmentLoadOp format);
		static VkAttachmentStoreOp Convert(AttachmentStoreOp format);
		static VkImageLayout Convert(ImageLayout format);
		static SampleCount Convert(uint8_t samples);
		static VkFormat Convert(ElementDataType type, uint32_t num, bool normalized);
		static VkPipelineBindPoint Convert(PipelineBindPoint bindPoint);
		static void PrintDeviceProps(VkPhysicalDeviceProperties props);
	};
}
