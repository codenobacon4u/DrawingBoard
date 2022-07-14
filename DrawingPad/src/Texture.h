#pragma once

#include "Utils.h"
#include "Buffer.h"

enum class BindFlags : uint32_t
{
	None				= 0x0,
	VertexBuffer		= 0x1,
	IndexBuffer			= 0x2,
	UniformBuffer		= 0x4,
	ShaderResource		= 0x8,
	StreamOutput		= 0x10,
	RenderTarget		= 0x20,
	DepthStencil		= 0x40,
	UnorderedAccess		= 0x80,
	IndirectDrawArgs	= 0x100,
	InputAttachment		= 0x200,
	RayTracing			= 0x400,
	SwapChain			= 0x600
};

inline constexpr BindFlags operator&(BindFlags x, BindFlags y) {
	return static_cast<BindFlags>(static_cast<uint32_t>(x) & static_cast<uint32_t>(y));
}

inline constexpr BindFlags operator|(BindFlags x, BindFlags y) {
	return static_cast<BindFlags>(static_cast<uint32_t>(x) | static_cast<uint32_t>(y));
}

inline constexpr BindFlags& operator&=(BindFlags& x, BindFlags y) {
	x = x & y;
	return x;
}

inline constexpr BindFlags& operator|=(BindFlags& x, BindFlags y) {
	x = x | y;
	return x;
}

enum class Usage
{
	Immutable,
	Default,
	Dynamic,
	Staging,
	Unified,
};

enum class TextureFormat : uint32_t {
	None,
	Unknown,

	R8Uint,
	R8Unorm,
	R8Sint,
	R8Snorm,

	RG8Uint,
	RG8Unorm,
	RG8Sint,
	RG8Snorm,

	RGB8Uint,
	RGB8Unorm,
	RGB8Sint,
	RGB8Snorm,

	RGBA8Uint,
	RGBA8Unorm,
	RGBA8Sint,
	RGBA8Snorm,

	BGR8Uint,
	BGR8Unorm,
	BGR8Sint,
	BGR8Snorm,

	BGRA8Uint,
	BGRA8Unorm,
	BGRA8Sint,
	BGRA8Snorm,

	R16Uint,
	R16Unorm,
	R16Sint,
	R16Sfloat,
	R16Snorm,

	RG16Uint,
	RG16Unorm,
	RG16Sint,
	RG16Sfloat,
	RG16Snorm,

	RGB16Uint,
	RGB16Unorm,
	RGB16Sint,
	RGB16Sfloat,
	RGB16Snorm,

	RGBA16Uint,
	RGBA16Unorm,
	RGBA16Sint,
	RGBA16Sfloat,
	RGBA16Snorm,

	R32Uint,
	R32Sint,
	R32Sfloat,

	RG32Uint,
	RG32Sint,
	RG32Sfloat,

	RGB32Uint,
	RGB32Sint,
	RGB32Sfloat,

	RGBA32Uint,
	RGBA32Sint,
	RGBA32Sfloat,

	D32Float,
	D32FloatS8Uint,
	D24UnormS8Uint,
	RGBA8UnormSRGB,
	BGRA8UnormSRGB,
};

enum class TextureType {
	Undefined,
	Buffer,
	DimTex1D,
	DimTex1DArray,
	DimTex2D,
	DimTex2DArray,
	DimTex3D,
};

enum class ViewType {
	Undefined,
	ShaderResource,
	RenderTarget,
	DepthStencil,
	UnorderedAccess
};

typedef struct TextureDesc {
	TextureType Type = TextureType::Undefined;
	uint32_t Width = 0;
	uint32_t Height = 0;
	union
	{
		uint32_t ArraySize = 1;
		uint32_t Depth;
	};
	TextureFormat Format = TextureFormat::Unknown;

	uint32_t MipLevels = 1;
	uint32_t SampleCount = 1;
	Usage Usage = Usage::Default;
	BindFlags BindFlags = BindFlags::None;
	AccessFlags AccessFlags = AccessFlags::None;

	TextureDesc() = default;
} TextureDesc;

typedef struct TextureViewDesc {
	ViewType ViewType = ViewType::Undefined;
	TextureFormat Format = TextureFormat::Unknown;
	TextureType Dim = TextureType::Undefined;
	uint32_t HighestMip = 0;
	uint32_t NumMipLevels = 1;
	uint32_t FirstSlice = 0;
	uint32_t Slices = 0;
} TextureViewDesc;

//TODO Might also follow similar path as BufferData and BufferLayout in Paperworks
typedef struct TextureData {
	TextureData() = default;
} TextureData;

class TextureView;
class Texture
{
public:
	Texture(const TextureDesc& desc)
		: m_Desc(desc)
	{}
	virtual ~Texture() {}

	virtual TextureView* CreateView(const TextureViewDesc& desc) = 0;

	const TextureDesc& GetDesc() { return m_Desc; }

	TextureView* GetDefaultView() { return m_DefaultView; }

protected:
	TextureDesc m_Desc;
	TextureView* m_DefaultView = nullptr;
};

class TextureView
{
public:
	TextureView(const TextureViewDesc& desc)
		: m_Desc(desc)
	{}

	virtual ~TextureView() {}

	virtual Texture* GetTexture() = 0;

	const TextureViewDesc& GetDesc() { return m_Desc; }

protected:
	TextureViewDesc m_Desc;
};
