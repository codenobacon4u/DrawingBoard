#include "pwpch.h"
#include "UtilsVK.h"

#include <fstream>

#define VMA_IMPLEMENTATION
#include <vma_mem_alloc.h>

namespace Vulkan
{
	void UtilsVK::Log(std::string path, std::string msg)
	{
		std::ofstream ofs(path.c_str(), std::ios_base::out | std::ios_base::app);
		ofs << msg << '\n';
		ofs.close();
	}

	VkFormat Vulkan::UtilsVK::Convert(TextureFormat format)
	{
		switch (format)
		{
		case TextureFormat::Unknown:
			return VK_FORMAT_UNDEFINED;
		case TextureFormat::R8Uint:
			return VK_FORMAT_R8_UINT;
		case TextureFormat::R8Unorm:
			return VK_FORMAT_R8_UNORM;
		case TextureFormat::R8Sint:
			return VK_FORMAT_R8_SINT;
		case TextureFormat::R8Snorm:
			return VK_FORMAT_R8_SNORM;
		case TextureFormat::RG8Uint:
			return VK_FORMAT_R8G8_UINT;
		case TextureFormat::RG8Unorm:
			return VK_FORMAT_R8G8_UNORM;
		case TextureFormat::RG8Sint:
			return VK_FORMAT_R8G8_SINT;
		case TextureFormat::RG8Snorm:
			return VK_FORMAT_R8G8_SNORM;
		case TextureFormat::RGB8Uint:
			return VK_FORMAT_R8G8B8_UINT;
		case TextureFormat::RGB8Unorm:
			return VK_FORMAT_R8G8B8_UNORM;
		case TextureFormat::RGB8Sint:
			return VK_FORMAT_R8G8B8_SINT;
		case TextureFormat::RGB8Snorm:
			return VK_FORMAT_R8G8B8_SNORM;
		case TextureFormat::RGBA8Uint:
			return VK_FORMAT_R8G8B8A8_UINT;
		case TextureFormat::RGBA8Unorm:
			return VK_FORMAT_R8G8B8A8_UNORM;
		case TextureFormat::RGBA8Sint:
			return VK_FORMAT_R8G8B8A8_SINT;
		case TextureFormat::RGBA8Snorm:
			return VK_FORMAT_R8G8B8A8_SNORM;
		case TextureFormat::BGR8Uint:
			return VK_FORMAT_B8G8R8_UINT;
		case TextureFormat::BGR8Unorm:
			return VK_FORMAT_B8G8R8_UNORM;
		case TextureFormat::BGR8Sint:
			return VK_FORMAT_B8G8R8_SINT;
		case TextureFormat::BGR8Snorm:
			return VK_FORMAT_B8G8R8_SNORM;
		case TextureFormat::BGRA8Uint:
			return VK_FORMAT_B8G8R8A8_UINT;
		case TextureFormat::BGRA8Unorm:
			return VK_FORMAT_B8G8R8A8_UNORM;
		case TextureFormat::BGRA8Sint:
			return VK_FORMAT_B8G8R8A8_SINT;
		case TextureFormat::BGRA8Snorm:
			return VK_FORMAT_B8G8R8A8_SNORM;
		case TextureFormat::R16Uint:
			return VK_FORMAT_R16_UINT;
		case TextureFormat::R16Unorm:
			return VK_FORMAT_R16_UNORM;
		case TextureFormat::R16Sint:
			return VK_FORMAT_R16_SINT;
		case TextureFormat::R16Sfloat:
			return VK_FORMAT_R16_SFLOAT;
		case TextureFormat::R16Snorm:
			return VK_FORMAT_R16_SNORM;
		case TextureFormat::RG16Uint:
			return VK_FORMAT_R16G16_UINT;
		case TextureFormat::RG16Unorm:
			return VK_FORMAT_R16G16_UNORM;
		case TextureFormat::RG16Sint:
			return VK_FORMAT_R16G16_SINT;
		case TextureFormat::RG16Sfloat:
			return VK_FORMAT_R16G16_SFLOAT;
		case TextureFormat::RG16Snorm:
			return VK_FORMAT_R16G16_SNORM;
		case TextureFormat::RGB16Uint:
			return VK_FORMAT_R16G16B16_UINT;
		case TextureFormat::RGB16Unorm:
			return VK_FORMAT_R16G16B16_UNORM;
		case TextureFormat::RGB16Sint:
			return VK_FORMAT_R16G16B16_SINT;
		case TextureFormat::RGB16Sfloat:
			return VK_FORMAT_R16G16B16_SFLOAT;
		case TextureFormat::RGB16Snorm:
			return VK_FORMAT_R16G16B16_SNORM;
		case TextureFormat::RGBA16Uint:
			return VK_FORMAT_R16G16B16A16_UINT;
		case TextureFormat::RGBA16Unorm:
			return VK_FORMAT_R16G16B16A16_UNORM;
		case TextureFormat::RGBA16Sint:
			return VK_FORMAT_R16G16B16A16_SINT;
		case TextureFormat::RGBA16Sfloat:
			return VK_FORMAT_R16G16B16A16_SFLOAT;
		case TextureFormat::RGBA16Snorm:
			return VK_FORMAT_R16G16B16A16_SNORM;
		case TextureFormat::R32Uint:
			return VK_FORMAT_R32_UINT;
		case TextureFormat::R32Sint:
			return VK_FORMAT_R32_SINT;
		case TextureFormat::R32Sfloat:
			return VK_FORMAT_R32_SFLOAT;
		case TextureFormat::RG32Uint:
			return VK_FORMAT_R32G32_UINT;
		case TextureFormat::RG32Sint:
			return VK_FORMAT_R32G32_SINT;
		case TextureFormat::RG32Sfloat:
			return VK_FORMAT_R32G32_SFLOAT;
		case TextureFormat::RGB32Uint:
			return VK_FORMAT_R32G32B32_UINT;
		case TextureFormat::RGB32Sint:
			return VK_FORMAT_R32G32B32_SINT;
		case TextureFormat::RGB32Sfloat:
			return VK_FORMAT_R32G32B32_SFLOAT;
		case TextureFormat::RGBA32Uint:
			return VK_FORMAT_R32G32B32A32_UINT;
		case TextureFormat::RGBA32Sint:
			return VK_FORMAT_R32G32B32A32_SINT;
		case TextureFormat::RGBA32Sfloat:
			return VK_FORMAT_R32G32B32A32_SFLOAT;
		case TextureFormat::D32Float:
			return VK_FORMAT_D32_SFLOAT;
		case TextureFormat::D32FloatS8Uint:
			return VK_FORMAT_D32_SFLOAT_S8_UINT;
		case TextureFormat::D24UnormS8Uint:
			return VK_FORMAT_D24_UNORM_S8_UINT;
		case TextureFormat::RGBA8UnormSRGB:
			return VK_FORMAT_R8G8B8A8_SRGB;
		case TextureFormat::BGRA8UnormSRGB:
			return VK_FORMAT_B8G8R8A8_SRGB;
		default:
			return VK_FORMAT_UNDEFINED;
		}
	}
	TextureFormat UtilsVK::Convert(VkFormat format)
	{
		switch (format)
		{
		case VK_FORMAT_UNDEFINED:
			return TextureFormat::Unknown;
		case VK_FORMAT_R8_UINT:
			return TextureFormat::R8Uint;
		case VK_FORMAT_R8_UNORM:
			return TextureFormat::R8Unorm;
		case VK_FORMAT_R8_SINT:
			return TextureFormat::R8Sint;
		case VK_FORMAT_R8_SNORM:
			return TextureFormat::R8Snorm;
		case VK_FORMAT_R8G8_UINT:
			return TextureFormat::RG8Uint;
		case VK_FORMAT_R8G8_UNORM:
			return TextureFormat::RG8Unorm;
		case VK_FORMAT_R8G8_SINT:
			return TextureFormat::RG8Sint;
		case VK_FORMAT_R8G8_SNORM:
			return TextureFormat::RG8Snorm;
		case VK_FORMAT_R8G8B8_UINT:
			return TextureFormat::RGB8Uint;
		case VK_FORMAT_R8G8B8_UNORM:
			return TextureFormat::RGB8Unorm;
		case VK_FORMAT_R8G8B8_SINT:
			return TextureFormat::RGB8Sint;
		case VK_FORMAT_R8G8B8_SNORM:
			return TextureFormat::RGB8Snorm;
		case VK_FORMAT_R8G8B8A8_UINT:
			return TextureFormat::RGBA8Uint;
		case VK_FORMAT_R8G8B8A8_UNORM:
			return TextureFormat::RGBA8Unorm;
		case VK_FORMAT_R8G8B8A8_SINT:
			return TextureFormat::RGBA8Sint;
		case VK_FORMAT_R8G8B8A8_SNORM:
			return TextureFormat::RGBA8Snorm;
		case VK_FORMAT_B8G8R8_UINT:
			return TextureFormat::BGR8Uint;
		case VK_FORMAT_B8G8R8_UNORM:
			return TextureFormat::BGR8Unorm;
		case VK_FORMAT_B8G8R8_SINT:
			return TextureFormat::BGR8Sint;
		case VK_FORMAT_B8G8R8_SNORM:
			return TextureFormat::BGR8Snorm;
		case VK_FORMAT_B8G8R8A8_UINT:
			return TextureFormat::BGRA8Uint;
		case VK_FORMAT_B8G8R8A8_UNORM:
			return TextureFormat::BGRA8Unorm;
		case VK_FORMAT_B8G8R8A8_SINT:
			return TextureFormat::BGRA8Sint;
		case VK_FORMAT_B8G8R8A8_SNORM:
			return TextureFormat::BGRA8Snorm;
		case VK_FORMAT_R16_UINT:
			return TextureFormat::R16Uint;
		case VK_FORMAT_R16_UNORM:
			return TextureFormat::R16Unorm;
		case VK_FORMAT_R16_SINT:
			return TextureFormat::R16Sint;
		case VK_FORMAT_R16_SFLOAT:
			return TextureFormat::R16Sfloat;
		case VK_FORMAT_R16_SNORM:
			return TextureFormat::R16Snorm;
		case VK_FORMAT_R16G16_UINT:
			return TextureFormat::RG16Uint;
		case VK_FORMAT_R16G16_UNORM:
			return TextureFormat::RG16Unorm;
		case VK_FORMAT_R16G16_SINT:
			return TextureFormat::RG16Sint;
		case VK_FORMAT_R16G16_SFLOAT:
			return TextureFormat::RG16Sfloat;
		case VK_FORMAT_R16G16_SNORM:
			return TextureFormat::RG16Snorm;
		case VK_FORMAT_R16G16B16_UINT:
			return TextureFormat::RGB16Uint;
		case VK_FORMAT_R16G16B16_UNORM:
			return TextureFormat::RGB16Unorm;
		case VK_FORMAT_R16G16B16_SINT:
			return TextureFormat::RGB16Sint;
		case VK_FORMAT_R16G16B16_SFLOAT:
			return TextureFormat::RGB16Sfloat;
		case VK_FORMAT_R16G16B16_SNORM:
			return TextureFormat::RGB16Snorm;
		case VK_FORMAT_R16G16B16A16_UINT:
			return TextureFormat::RGBA16Uint;
		case VK_FORMAT_R16G16B16A16_UNORM:
			return TextureFormat::RGBA16Unorm;
		case VK_FORMAT_R16G16B16A16_SINT:
			return TextureFormat::RGBA16Sint;
		case VK_FORMAT_R16G16B16A16_SFLOAT:
			return TextureFormat::RGBA16Sfloat;
		case VK_FORMAT_R16G16B16A16_SNORM:
			return TextureFormat::RGBA16Snorm;
		case VK_FORMAT_R32_UINT:
			return TextureFormat::R32Uint;
		case VK_FORMAT_R32_SINT:
			return TextureFormat::R32Sint;
		case VK_FORMAT_R32_SFLOAT:
			return TextureFormat::R32Sfloat;
		case VK_FORMAT_R32G32_UINT:
			return TextureFormat::RG32Uint;
		case VK_FORMAT_R32G32_SINT:
			return TextureFormat::RG32Sint;
		case VK_FORMAT_R32G32_SFLOAT:
			return TextureFormat::RG32Sfloat;
		case VK_FORMAT_R32G32B32_UINT:
			return TextureFormat::RGB32Uint;
		case VK_FORMAT_R32G32B32_SINT:
			return TextureFormat::RGB32Sint;
		case VK_FORMAT_R32G32B32_SFLOAT:
			return TextureFormat::RGB32Sfloat;
		case VK_FORMAT_R32G32B32A32_UINT:
			return TextureFormat::RGBA32Uint;
		case VK_FORMAT_R32G32B32A32_SINT:
			return TextureFormat::RGBA32Sint;
		case VK_FORMAT_R32G32B32A32_SFLOAT:
			return TextureFormat::RGBA32Sfloat;
		case VK_FORMAT_D32_SFLOAT:
			return TextureFormat::D32Float;
		case VK_FORMAT_D32_SFLOAT_S8_UINT:
			return TextureFormat::D32FloatS8Uint;
		case VK_FORMAT_D24_UNORM_S8_UINT:
			return TextureFormat::D24UnormS8Uint;
		case VK_FORMAT_R8G8B8A8_SRGB:
			return TextureFormat::RGBA8UnormSRGB;
		case VK_FORMAT_B8G8R8A8_SRGB:
			return TextureFormat::BGRA8UnormSRGB;
		default:
			return TextureFormat::Unknown;
		}
	}
	VkAttachmentLoadOp UtilsVK::Convert(AttachmentLoadOp op)
	{
		switch (op)
		{
		case AttachmentLoadOp::Load:
			return VK_ATTACHMENT_LOAD_OP_LOAD;
		case AttachmentLoadOp::Clear:
			return VK_ATTACHMENT_LOAD_OP_CLEAR;
		case AttachmentLoadOp::Discard:
		case AttachmentLoadOp::DontCare:
			return VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		default:
			throw DBG_NEW std::runtime_error("Failed to convert attachment load op");
		}
	}
	VkAttachmentStoreOp UtilsVK::Convert(AttachmentStoreOp op)
	{
		switch (op)
		{
		case AttachmentStoreOp::Store:
			return VK_ATTACHMENT_STORE_OP_STORE;
		case AttachmentStoreOp::Discard:
		case AttachmentStoreOp::DontCare:
			return VK_ATTACHMENT_STORE_OP_DONT_CARE;
		default:
			throw DBG_NEW std::runtime_error("Failed to convert attachment store op");
		}
	}
	VkImageLayout UtilsVK::Convert(ImageLayout layout)
	{
		switch (layout)
		{
		case ImageLayout::Undefined:
			return VK_IMAGE_LAYOUT_UNDEFINED;
		case ImageLayout::General:
			return VK_IMAGE_LAYOUT_GENERAL;
		case ImageLayout::ColorAttachOptimal:
			return VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
		case ImageLayout::DepthStencilAttachOptimal:
			return VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
		case ImageLayout::DepthStencilReadOnlyOptimal:
			return VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
		case ImageLayout::ShaderReadOnlyOptimal:
			return VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		case ImageLayout::TransferSrcOptimal:
			return VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
		case ImageLayout::TransferDstOptimal:
			return VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
		case ImageLayout::DepthReadOnlyStencilAttachOptimal:
			return VK_IMAGE_LAYOUT_DEPTH_READ_ONLY_STENCIL_ATTACHMENT_OPTIMAL;
		case ImageLayout::DepthAttachStencilReadOnlyOptimal:
			return VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_STENCIL_READ_ONLY_OPTIMAL;
		case ImageLayout::PresentSrcKHR:
			return VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
		case ImageLayout::DepthAttachOptimal:
			return VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL;
		default:
			return VK_IMAGE_LAYOUT_UNDEFINED;
		}
	}
	SampleCount UtilsVK::Convert(uint8_t samples)
	{
		switch (samples)
		{
		case 1:
			return SampleCount::e1Bit;
		case 2:
			return SampleCount::e2Bit;
		case 4:
			return SampleCount::e4Bit;
		case 8:
			return SampleCount::e8Bit;
		case 16:
			return SampleCount::e16Bit;
		case 32:
			return SampleCount::e32Bit;
		case 64:
			return SampleCount::e64Bit;
		default:
			throw DBG_NEW std::runtime_error("Invalid sample count");
		}
	}

	VkFormat UtilsVK::Convert(ElementDataType type, uint32_t num, bool normalized)
	{
		switch (type)
		{
		case ElementDataType::Float16:
			if (normalized)
			{
				return VK_FORMAT_UNDEFINED;
			}
			else
			{
				switch (num)
				{
				case 1: return VK_FORMAT_R16_SFLOAT;
				case 2: return VK_FORMAT_R16G16_SFLOAT;
				case 3: return VK_FORMAT_R16G16B16_SFLOAT;
				case 4: return VK_FORMAT_R16G16B16A16_SFLOAT;
				default: return VK_FORMAT_UNDEFINED;
				}
			}
		case ElementDataType::Float32:
			if (normalized)
			{
				return VK_FORMAT_UNDEFINED;
			}
			else
			{
				switch (num)
				{
				case 1: return VK_FORMAT_R32_SFLOAT;
				case 2: return VK_FORMAT_R32G32_SFLOAT;
				case 3: return VK_FORMAT_R32G32B32_SFLOAT;
				case 4: return VK_FORMAT_R32G32B32A32_SFLOAT;
				default: return VK_FORMAT_UNDEFINED;
				}
			}
		case ElementDataType::Int8:
			if (normalized)
			{
				switch (num)
				{
				case 1: return VK_FORMAT_R8_SNORM;
				case 2: return VK_FORMAT_R8G8_SNORM;
				case 3: return VK_FORMAT_R8G8B8_SNORM;
				case 4: return VK_FORMAT_R8G8B8A8_SNORM;
				default: return VK_FORMAT_UNDEFINED;
				}
			}
			else
			{
				switch (num)
				{
				case 1: return VK_FORMAT_R8_SINT;
				case 2: return VK_FORMAT_R8G8_SINT;
				case 3: return VK_FORMAT_R8G8B8_SINT;
				case 4: return VK_FORMAT_R8G8B8A8_SINT;
				default: return VK_FORMAT_UNDEFINED;
				}
			}
		case ElementDataType::Int16:
			if (normalized)
			{
				switch (num)
				{
				case 1: return VK_FORMAT_R16_SNORM;
				case 2: return VK_FORMAT_R16G16_SNORM;
				case 3: return VK_FORMAT_R16G16B16_SNORM;
				case 4: return VK_FORMAT_R16G16B16A16_SNORM;
				default: return VK_FORMAT_UNDEFINED;
				}
			}
			else
			{
				switch (num)
				{
				case 1: return VK_FORMAT_R16_SINT;
				case 2: return VK_FORMAT_R16G16_SINT;
				case 3: return VK_FORMAT_R16G16B16_SINT;
				case 4: return VK_FORMAT_R16G16B16A16_SINT;
				default: return VK_FORMAT_UNDEFINED;
				}
			}
		case ElementDataType::Int32:
			switch (num)
			{
			case 1: return VK_FORMAT_R32_SINT;
			case 2: return VK_FORMAT_R32G32_SINT;
			case 3: return VK_FORMAT_R32G32B32_SINT;
			case 4: return VK_FORMAT_R32G32B32A32_SINT;
			default: return VK_FORMAT_UNDEFINED;
			}
		case ElementDataType::Uint8:
			if (normalized)
			{
				switch (num)
				{
				case 1: return VK_FORMAT_R8_UNORM;
				case 2: return VK_FORMAT_R8G8_UNORM;
				case 3: return VK_FORMAT_R8G8B8_UNORM;
				case 4: return VK_FORMAT_R8G8B8A8_UNORM;
				default: return VK_FORMAT_UNDEFINED;
				}
			}
			else
			{
				switch (num)
				{
				case 1: return VK_FORMAT_R8_UINT;
				case 2: return VK_FORMAT_R8G8_UINT;
				case 3: return VK_FORMAT_R8G8B8_UINT;
				case 4: return VK_FORMAT_R8G8B8A8_UINT;
				default: return VK_FORMAT_UNDEFINED;
				}
			}
		case ElementDataType::Uint16:
			if (normalized)
			{
				switch (num)
				{
				case 1: return VK_FORMAT_R16_UNORM;
				case 2: return VK_FORMAT_R16G16_UNORM;
				case 3: return VK_FORMAT_R16G16B16_UNORM;
				case 4: return VK_FORMAT_R16G16B16A16_UNORM;
				default: return VK_FORMAT_UNDEFINED;
				}
			}
			else
			{
				switch (num)
				{
				case 1: return VK_FORMAT_R16_UINT;
				case 2: return VK_FORMAT_R16G16_UINT;
				case 3: return VK_FORMAT_R16G16B16_UINT;
				case 4: return VK_FORMAT_R16G16B16A16_UINT;
				default: return VK_FORMAT_UNDEFINED;
				}
			}
		case ElementDataType::Uint32:
			switch (num)
			{
			case 1: return VK_FORMAT_R32_UINT;
			case 2: return VK_FORMAT_R32G32_UINT;
			case 3: return VK_FORMAT_R32G32B32_UINT;
			case 4: return VK_FORMAT_R32G32B32A32_UINT;
			default: return VK_FORMAT_UNDEFINED;
			}
		default: return VK_FORMAT_UNDEFINED;
		}
	}

	VkPipelineBindPoint UtilsVK::Convert(PipelineBindPoint bindPoint)
	{
		switch (bindPoint) 
		{
		case PipelineBindPoint::Graphics:
			return VK_PIPELINE_BIND_POINT_GRAPHICS;
		case PipelineBindPoint::Compute:
			return VK_PIPELINE_BIND_POINT_GRAPHICS;
		default:
			return VK_PIPELINE_BIND_POINT_MAX_ENUM;
		}
	}

	void UtilsVK::PrintDeviceProps(VkPhysicalDeviceProperties props)
	{
		std::string vendor, type, driver;
		switch (props.vendorID) {
		case 0x1002:
			vendor = "AMD";
			break;
		case 0x1010:
			vendor = "ImgTec";
			break;
		case 0x10DE:
			vendor = "NVIDIA";
			break;
		case 0x13B5:
			vendor = "ARM";
			break;
		case 0x5143:
			vendor = "Qualcomm";
			break;
		case 0x8086:
			vendor = "Intel";
			break;
		default:
			vendor = "UNKOWN";
			break;
		}
		switch (props.deviceType) {
		case VkPhysicalDeviceType::VK_PHYSICAL_DEVICE_TYPE_CPU:
			type = "CPU";
			break;
		case VkPhysicalDeviceType::VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU:
			type = "Discrete";
			break;
		case VkPhysicalDeviceType::VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU:
			type = "Integrated";
			break;
		case VkPhysicalDeviceType::VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU:
			type = "Virtual";
			break;
		case VkPhysicalDeviceType::VK_PHYSICAL_DEVICE_TYPE_OTHER:
		default:
			type = "Other";
			break;
		}
		static std::string const values[] = {
			"",
			"AMD Proprietary",
			"AMD Open Source",
			"MESA RADV",
			"NVIDIA Proprietary",
			"INTEL Proprietary Windows",
			"INTEL Open Source MESA",
			"Imagination Proprietary",
			"Qualcomm Proprietary",
			"ARM Proprietary",
			"Google SWIFTSHADER",
			"GGP Proprietary",
			"BROADCOM Proprietary",
			"MESA LLVMPIPE",
			"MOLTENVK",
		};
		driver = props.driverVersion < values->size() ? values[props.driverVersion] : "UNKOWN";
		std::string api = string_format("%d.%d.%d", props.apiVersion >> 22, (props.apiVersion >> 12) & 0x3ff, props.apiVersion & 0xfff);
		std::cout
			<< "Device Name: " << props.deviceName << "\n"
			<< "Device Type: " << type << "\n"
			<< "Driver Version: " << driver << "\n"
			<< "Vulkan Version: " << api << "\n"
			<< "Vender ID: " << vendor << "\n"
			<< "Device ID: " << props.deviceID << "\n";
	}
}
