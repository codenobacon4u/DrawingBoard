#include "dppch.h"
#include "DebugVK.h"

namespace DrawingPad
{
	static bool active = false;
	static bool extensionPresent = false;
	static bool debug = false;

	static PFN_vkDebugMarkerSetObjectTagEXT vkDebugMarkerSetObjectTag = VK_NULL_HANDLE;
	static PFN_vkDebugMarkerSetObjectNameEXT vkDebugMarkerSetObjectName = VK_NULL_HANDLE;
	static PFN_vkCmdDebugMarkerBeginEXT vkCmdDebugMarkerBegin = VK_NULL_HANDLE;
	static PFN_vkCmdDebugMarkerEndEXT vkCmdDebugMarkerEnd = VK_NULL_HANDLE;
	static PFN_vkCmdDebugMarkerInsertEXT vkCmdDebugMarkerInsert = VK_NULL_HANDLE;
	static PFN_vkSetDebugUtilsObjectNameEXT SetDebugUtilsObjectNameEXT = VK_NULL_HANDLE;
	static PFN_vkSetDebugUtilsObjectTagEXT SetDebugUtilsObjectTagEXT = VK_NULL_HANDLE;

	void Vulkan::DebugMarker::Setup(VkDevice device, VkPhysicalDevice physicalDevice)
	{
		// Check if the debug marker extension is present (which is the case if run from a graphics debugger)
		uint32_t extensionCount;
		vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &extensionCount, nullptr);
		std::vector<VkExtensionProperties> extensions(extensionCount);
		vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &extensionCount, extensions.data());
		for (const auto& extension : extensions) {
			if (strcmp(extension.extensionName, VK_EXT_DEBUG_MARKER_EXTENSION_NAME) == 0) {
				extensionPresent = true;
				break;
			}
		}

		if (extensionPresent) {
			// The debug marker extension is not part of the core, so function pointers need to be loaded manually
			vkDebugMarkerSetObjectTag = (PFN_vkDebugMarkerSetObjectTagEXT)vkGetDeviceProcAddr(device, "vkDebugMarkerSetObjectTagEXT");
			vkDebugMarkerSetObjectName = (PFN_vkDebugMarkerSetObjectNameEXT)vkGetDeviceProcAddr(device, "vkDebugMarkerSetObjectNameEXT");
			vkCmdDebugMarkerBegin = (PFN_vkCmdDebugMarkerBeginEXT)vkGetDeviceProcAddr(device, "vkCmdDebugMarkerBeginEXT");
			vkCmdDebugMarkerEnd = (PFN_vkCmdDebugMarkerEndEXT)vkGetDeviceProcAddr(device, "vkCmdDebugMarkerEndEXT");
			vkCmdDebugMarkerInsert = (PFN_vkCmdDebugMarkerInsertEXT)vkGetDeviceProcAddr(device, "vkCmdDebugMarkerInsertEXT");
			// Set flag if at least one function pointer is present
			active = (vkDebugMarkerSetObjectName != VK_NULL_HANDLE);
		}
		else
		{
			std::cout << "Warning: " << VK_EXT_DEBUG_MARKER_EXTENSION_NAME << " not present, debug markers are disabled.";
			std::cout << "Try running from inside a Vulkan graphics debugger (e.g. RenderDoc)" << std::endl;
		}
		SetDebugUtilsObjectNameEXT = (PFN_vkSetDebugUtilsObjectNameEXT)vkGetDeviceProcAddr(device, "vkSetDebugUtilsObjectNameEXT");
		SetDebugUtilsObjectTagEXT = (PFN_vkSetDebugUtilsObjectTagEXT)vkGetDeviceProcAddr(device, "vkSetDebugUtilsObjectTagEXT");
		debug = true;
	}

	void Vulkan::DebugMarker::SetName(VkDevice device, uint64_t object, VkObjectType objectType, std::string name)
	{
		// Check for valid function pointer (may not be present if not running in a debugging application)
		if (active)
		{
			VkDebugMarkerObjectNameInfoEXT nameInfo = {};
			nameInfo.sType = VK_STRUCTURE_TYPE_DEBUG_MARKER_OBJECT_NAME_INFO_EXT;
			nameInfo.objectType = (VkDebugReportObjectTypeEXT)objectType;
			nameInfo.object = object;
			nameInfo.pObjectName = name.c_str();
			vkDebugMarkerSetObjectName(device, &nameInfo);
		}
		else if (debug)
		{
			VkDebugUtilsObjectNameInfoEXT nameInfo = {};
			nameInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT;
			nameInfo.pNext = nullptr;
			nameInfo.objectType = objectType;
			nameInfo.objectHandle = object;
			nameInfo.pObjectName = name.c_str();
			SetDebugUtilsObjectNameEXT(device, &nameInfo);
		}
	}

	void Vulkan::DebugMarker::SetObjectTag(VkDevice device, uint64_t object, VkObjectType objectType, uint64_t name, size_t tagSize, std::string tag)
	{
		// Check for valid function pointer (may not be present if not running in a debugging application)
		if (active)
		{
			VkDebugMarkerObjectTagInfoEXT tagInfo = {};
			tagInfo.sType = VK_STRUCTURE_TYPE_DEBUG_MARKER_OBJECT_TAG_INFO_EXT;
			tagInfo.objectType = (VkDebugReportObjectTypeEXT)objectType;
			tagInfo.object = object;
			tagInfo.tagName = name;
			tagInfo.tagSize = tagSize;
			tagInfo.pTag = tag.c_str();
			vkDebugMarkerSetObjectTag(device, &tagInfo);
		}
		else if (debug)
		{
			VkDebugUtilsObjectTagInfoEXT tagInfo = {};
			tagInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_TAG_INFO_EXT;
			tagInfo.pNext = nullptr;
			tagInfo.objectType = objectType;
			tagInfo.objectHandle = object;
			tagInfo.tagName = name;
			tagInfo.tagSize = tagSize;
			tagInfo.pTag = tag.c_str();
			SetDebugUtilsObjectTagEXT(device, &tagInfo);
		}
	}

	void Vulkan::DebugMarker::BeginRegion(VkCommandBuffer cmdbuffer, const char* pMarkerName, float* color)
	{
		// Check for valid function pointer (may not be present if not running in a debugging application)
		if (active)
		{
			VkDebugMarkerMarkerInfoEXT markerInfo = {};
			markerInfo.sType = VK_STRUCTURE_TYPE_DEBUG_MARKER_MARKER_INFO_EXT;
			memcpy(markerInfo.color, &color[0], sizeof(float) * 4);
			markerInfo.pMarkerName = pMarkerName;
			vkCmdDebugMarkerBegin(cmdbuffer, &markerInfo);
		}
	}

	void Vulkan::DebugMarker::Insert(VkCommandBuffer cmdbuffer, std::string markerName, float* color)
	{
		// Check for valid function pointer (may not be present if not running in a debugging application)
		if (active)
		{
			VkDebugMarkerMarkerInfoEXT markerInfo = {};
			markerInfo.sType = VK_STRUCTURE_TYPE_DEBUG_MARKER_MARKER_INFO_EXT;
			memcpy(markerInfo.color, &color[0], sizeof(float) * 4);
			markerInfo.pMarkerName = markerName.c_str();
			vkCmdDebugMarkerInsert(cmdbuffer, &markerInfo);
		}
	}

	void Vulkan::DebugMarker::EndRegion(VkCommandBuffer cmdBuffer)
	{
		// Check for valid function (may not be present if not running in a debugging application)
		if (vkCmdDebugMarkerEnd)
		{
			vkCmdDebugMarkerEnd(cmdBuffer);
		}
	}
}
