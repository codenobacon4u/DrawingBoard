#include "pwpch.h"
#include "RenderPassPoolVK.h"

#include "GraphicsDeviceVK.h"
#include "UtilsVK.h"

#include <chrono>
#include <array>

namespace VkAPI 
{
	using FloatingPointMicroseconds = std::chrono::duration<double, std::micro>;
	bool RPKey::operator==(const RPKey& rhs) const
	{
		if (GetHash() != rhs.GetHash() ||
			NumColors != rhs.NumColors ||
			SampleCount != rhs.SampleCount ||
			DepthFormat != rhs.DepthFormat)
			return false;
		for (uint32_t i = 0; i < NumColors; i++)
			if (ColorFormats[i] != rhs.ColorFormats[i])
				return false;
		return true;
	}

	size_t RPKey::GetHash() const
	{
		if (Hash == 0)
		{
			hash_combine(Hash, NumColors);
			hash_combine(Hash, SampleCount);
			hash_combine(Hash, DepthFormat);
			for (uint32_t i = 0; i < NumColors; i++)
				hash_combine(Hash, ColorFormats[i]);
		}
		return Hash;
	}

    RenderPassVK* RenderPassPoolVK::GetRenderPass(const RPKey& key)
    {
		HZ_PROFILE_FUNCTION();
#if 1
		auto it = m_Map.find(key);
		if (it == m_Map.end())
		{
			std::array<RenderPassAttachmentDesc, 9> attachments;
			std::array<AttachmentReference, 9> refs;
			SubpassDesc subpass;

			auto desc = RenderPassVK::GetDefaultDesc(key.NumColors, key.ColorFormats, key.DepthFormat, UtilsVK::Convert(key.SampleCount), attachments, refs, subpass);
			RenderPassVK* rp = m_Device.CreateRenderPass(desc);
			it = m_Map.emplace(key, std::move(rp)).first;
		}
#else
		auto it = m_Array.begin();
		for (; it != m_Array.end(); it++) {
			if (it->first == key)
				break;
		}

		if (it == m_Array.end())
		{
			std::array<RenderPassAttachmentDesc, 8> attachments;
			std::array<AttachmentReference, 8> refs;
			SubpassDesc subpass;

			auto desc = RenderPassVK::GetDefaultDesc(key.NumColors, key.ColorFormats, key.DepthFormat, UtilsVK::Convert(key.SampleCount), attachments, refs, subpass);
			RenderPassVK* rp = m_Device.CreateRenderPass(desc);
			it = m_Array.emplace_back(std::make_pair(key, std::move(rp))).first;
		}
#endif
		return it->second;
	}
}
