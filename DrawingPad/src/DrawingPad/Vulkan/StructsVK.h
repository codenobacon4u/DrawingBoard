#pragma once

#include <vulkan/vulkan.h>

#include <vector>

#include "DrawingPad/Shader.h"

namespace DrawingPad
{
	namespace Vulkan
	{
		struct BindingInfo {
			VkDescriptorBufferInfo bufferInfo;
			VkDescriptorImageInfo imageInfo;
			size_t hash;
		};

		struct DSLKey {
			std::vector<ShaderResourceBinding> Resources;

			DSLKey(std::vector<ShaderResourceBinding> resources);

			bool operator==(const DSLKey& rhs) const;
			size_t GetHash() const;
		private:
			mutable size_t Hash = 0;
		};

		struct FBKey {
			VkRenderPass Pass = VK_NULL_HANDLE;
			std::vector<VkFormat> Formats = {};
			std::vector<VkImageUsageFlags> Usages = {};

			bool operator==(const FBKey& rhs) const;
			size_t GetHash() const;

		private:
			mutable size_t Hash = 0;
		};

		class DescriptorSetPoolVK;
		class CommandPoolVK;
		struct FrameData {
			uint32_t Index = 0;
			std::vector<std::map<uint32_t, CommandPoolVK*>> CommandPools = {};
			DescriptorSetPoolVK* DescriptorPool = nullptr;
			VkSemaphore ImageAcquired = VK_NULL_HANDLE;
			VkSemaphore RenderFinished = VK_NULL_HANDLE;
			VkFence FrameFence = VK_NULL_HANDLE;
		};

		struct PoolSizes {
			std::vector<std::pair<VkDescriptorType, float>> sizes =
			{
				{ VK_DESCRIPTOR_TYPE_SAMPLER, 0.5f },
				{ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 4.f },
				{ VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 4.f },
				{ VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1.f },
				{ VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 1.f },
				{ VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 1.f },
				{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 2.f },
				{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 2.f },
				{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1.f },
				{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1.f },
				{ VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 0.5f }
			};
		};

		struct Queue {
			uint32_t familyIndex = 0;
			uint32_t index = 0;
			VkQueue queue = VK_NULL_HANDLE;
			VkBool32 presentSupport = VK_FALSE;
			VkQueueFamilyProperties properties = {};
		};

		struct SwapSupportDetails {
			VkSurfaceCapabilitiesKHR capabilities = {};
			std::vector<VkSurfaceFormatKHR> formats = {};
			std::vector<VkPresentModeKHR> presentModes = {};
		};

	}
}
