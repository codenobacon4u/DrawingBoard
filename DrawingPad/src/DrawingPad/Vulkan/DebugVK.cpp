#include "dppch.h"
#include "DebugVK.h"

namespace DrawingPad
{
	static bool active = false;
	static bool extensionPresent = false;
	static bool debug = false;

	static PFN_vkCmdBeginDebugUtilsLabelEXT vkCmdBeginDebugUtilsLabel = VK_NULL_HANDLE;
	static PFN_vkCmdEndDebugUtilsLabelEXT vkCmdEndDebugUtilsLabel = VK_NULL_HANDLE;
	static PFN_vkCmdInsertDebugUtilsLabelEXT vkCmdInsertDebugUtilsLabel = VK_NULL_HANDLE;
	static PFN_vkSetDebugUtilsObjectNameEXT vkSetDebugUtilsObjectNameEXT = VK_NULL_HANDLE;
	static PFN_vkSetDebugUtilsObjectTagEXT vkSetDebugUtilsObjectTagEXT = VK_NULL_HANDLE;

	void Vulkan::DebugMarker::Setup(VkDevice device, VkPhysicalDevice physicalDevice)
	{
		// Check if the debug marker extension is present (which is the case if run from a graphics debugger)
		uint32_t extensionCount;
		vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &extensionCount, nullptr);
		std::vector<VkExtensionProperties> extensions(extensionCount);
		vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &extensionCount, extensions.data());
		for (const auto& extension : extensions) {
			if (strcmp(extension.extensionName, VK_EXT_DEBUG_UTILS_EXTENSION_NAME) == 0) {
				extensionPresent = true;
				break;
			}
		}

		if (extensionPresent) {
			// The debug marker extension is not part of the core, so function pointers need to be loaded manually
			vkCmdBeginDebugUtilsLabel = (PFN_vkCmdBeginDebugUtilsLabelEXT)vkGetDeviceProcAddr(device, "vkCmdBeginDebugUtilsLabelEXT");
			vkCmdEndDebugUtilsLabel = (PFN_vkCmdEndDebugUtilsLabelEXT)vkGetDeviceProcAddr(device, "vkCmdEndDebugUtilsLabelEXT");
			vkCmdInsertDebugUtilsLabel = (PFN_vkCmdInsertDebugUtilsLabelEXT)vkGetDeviceProcAddr(device, "vkCmdInsertDebugUtilsLabelEXT");
			vkSetDebugUtilsObjectNameEXT = (PFN_vkSetDebugUtilsObjectNameEXT)vkGetDeviceProcAddr(device, "vkSetDebugUtilsObjectNameEXT");
			vkSetDebugUtilsObjectTagEXT = (PFN_vkSetDebugUtilsObjectTagEXT)vkGetDeviceProcAddr(device, "vkSetDebugUtilsObjectTagEXT");
			// Set flag if at least one function pointer is present
			active = (vkSetDebugUtilsObjectTagEXT != VK_NULL_HANDLE);
		}
		else
		{
			DP_WARN("Warning: {} not present, debug markers are disabled. Try running from inside a Vulkan graphics debugger (e.g. RenderDoc)", VK_EXT_DEBUG_MARKER_EXTENSION_NAME);
		}
	}

	void Vulkan::DebugMarker::SetName(VkDevice device, uint64_t object, VkObjectType objectType, std::string name)
	{
		// Check for valid function pointer (may not be present if not running in a debugging application)
		if (active)
		{
			VkDebugUtilsObjectNameInfoEXT nameInfo = {};
			nameInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT;
			nameInfo.pNext = nullptr;
			nameInfo.objectType = objectType;
			nameInfo.objectHandle = object;
			nameInfo.pObjectName = name.c_str();
			vkSetDebugUtilsObjectNameEXT(device, &nameInfo);
		}
	}

	void Vulkan::DebugMarker::SetObjectTag(VkDevice device, uint64_t object, VkObjectType objectType, uint64_t name, size_t tagSize, std::string& tag)
	{
		// Check for valid function pointer (may not be present if not running in a debugging application)
		if (active)
		{
			VkDebugUtilsObjectTagInfoEXT tagInfo = {};
			tagInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_TAG_INFO_EXT;
			tagInfo.pNext = nullptr;
			tagInfo.objectType = objectType;
			tagInfo.objectHandle = object;
			tagInfo.tagName = name;
			tagInfo.tagSize = tagSize;
			tagInfo.pTag = tag.c_str();
			vkSetDebugUtilsObjectTagEXT(device, &tagInfo);
		}
	}

	void Vulkan::DebugMarker::BeginRegion(VkCommandBuffer cmdbuffer, std::string& label, std::array<float, 4> color)
	{
		// Check for valid function pointer (may not be present if not running in a debugging application)
		if (active)
		{
			VkDebugUtilsLabelEXT labelInfo = {};
			labelInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_LABEL_EXT;
			labelInfo.pLabelName = label.c_str();
			labelInfo.color[0] = color[0];
			labelInfo.color[1] = color[1];
			labelInfo.color[2] = color[2];
			labelInfo.color[3] = color[3];
			vkCmdBeginDebugUtilsLabel(cmdbuffer, &labelInfo);
		}
	}

	void Vulkan::DebugMarker::Insert(VkCommandBuffer cmdbuffer, std::string& label, std::array<float, 4> color)
	{
		// Check for valid function pointer (may not be present if not running in a debugging application)
		if (active)
		{
			VkDebugUtilsLabelEXT labelInfo = {};
			labelInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_LABEL_EXT;
			labelInfo.pLabelName = label.c_str();
			labelInfo.color[0] = color[0];
			labelInfo.color[1] = color[1];
			labelInfo.color[2] = color[2];
			labelInfo.color[3] = color[3];
			vkCmdInsertDebugUtilsLabel(cmdbuffer, &labelInfo);
		}
	}

	void Vulkan::DebugMarker::EndRegion(VkCommandBuffer cmdBuffer)
	{
		// Check for valid function pointer (may not be present if not running in a debugging application)
		if (active)
		{
			vkCmdEndDebugUtilsLabel(cmdBuffer);
		}
	}
}
