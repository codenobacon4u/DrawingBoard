#pragma once

#include <vulkan/vulkan.h>

namespace Vulkan {
	class DebugMarker
	{
	public:
		static void Setup(VkDevice device, VkPhysicalDevice physicalDevice);

		static void SetName(VkDevice device, uint64_t object, VkObjectType objectType, std::string name);

		// Set the tag for an object
		static void SetObjectTag(VkDevice device, uint64_t object, VkObjectType objectType, uint64_t name, size_t tagSize, std::string tag);

		// Start a DBG_NEW debug marker region
		static void BeginRegion(VkCommandBuffer cmdbuffer, const char* pMarkerName, float* color);

		// Insert a DBG_NEW debug marker into the command buffer
		static void Insert(VkCommandBuffer cmdbuffer, std::string markerName, float* color);

		// End the current debug marker region
		static void EndRegion(VkCommandBuffer cmdBuffer);
	};
}
