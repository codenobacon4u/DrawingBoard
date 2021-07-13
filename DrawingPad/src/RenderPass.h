#pragma once
#include "Texture.h"

enum SampleCount : uint8_t{
	e1Bit = 1,
	e2Bit = 2,
	e4Bit = 4,
	e8Bit = 8,
	e16Bit = 16,
	e32Bit = 32,
	e64Bit = 64
};

enum class AttachmentLoadOp {
	Load,
	Clear,
	Discard,
	DontCare
};

enum class AttachmentStoreOp {
	Store,
	Discard,
	DontCare
};

enum class ImageLayout {
	Undefined,
	General,
	ColorAttachOptimal,
	DepthStencilAttachOptimal,
	DepthStencilReadOnlyOptimal,
	ShaderReadOnlyOptimal,
	TransferSrcOptimal,
	TransferDstOptimal,
	DepthReadOnlyStencilAttachOptimal,
	DepthAttachStencilReadOnlyOptimal,
	DepthAttachOptimal,
	DepthReadOnlyOptimal,
	StencilAttachOptimal,
	StencilReadOnlyOptimal,
	PresentSrcKHR,
};

enum PipelineBindPoint {
	Graphics = 0,
	Compute = 1,
	Raytracing = 1000165000
};

enum PipelineStage : uint32_t {
	Undefined = 0,
	Top = 1,
	DrawIndirect = 2,
	VertexInput = 4,
	VertexShader = 8,
	HullShader = 16,
	DomainShader = 32,
	GeometryShader = 64,
	FragmentShader = 128,
	EarlyFragTests = 256,
	LateFragTests = 512,
	ColorAttachOutput = 1024,
	ComputeShader = 2048,
	Transfer = 4096,
	Bottom = 8192,
	Host = 16384,
	AllGraphics = 32768,
	AllCommands = 65536,
	RaytracingShader = 2097152
};

enum SubpassAccess : uint32_t {
	NA					= 0x00000000,
	IndirectCmdRead			= 0x00000001,
	IndexRead				= 0x00000002,
	VertexAttribRead		= 0x00000004,
	UniformRead				= 0x00000008,
	InputAttachRead			= 0x00000010,
	ShaderRead				= 0x00000020,
	ShaderWrite				= 0x00000040,
	ColorAttachRead			= 0x00000080,
	ColorAttachWrite		= 0x00000100,
	DepthStencilAttachRead	= 0x00000200,
	DepthStencilAttachWrite	= 0x00000400,
	TransferRead			= 0x00000800,
	TransferWrite			= 0x00001000,
	HostRead				= 0x00002000,
	HostWrite				= 0x00004000,
	MemoryRead				= 0x00008000,
	MemoryWrite				= 0x00010000,
};

typedef struct AttachmentReference {
	uint32_t Attachment = 0;
	ImageLayout Layout = ImageLayout::Undefined;

	AttachmentReference() = default;
} AttachmentReference;

typedef struct RenderPassAttachmentDesc {
	TextureFormat Format = TextureFormat::Unknown;
	SampleCount Samples = SampleCount::e1Bit;
	AttachmentLoadOp LoadOp = AttachmentLoadOp::Load;
	AttachmentStoreOp StoreOp = AttachmentStoreOp::Store;
	AttachmentLoadOp StencilLoadOp = AttachmentLoadOp::Load;
	AttachmentStoreOp StencilStoreOp = AttachmentStoreOp::Store;
	ImageLayout InitialLayout = ImageLayout::Undefined;
	ImageLayout FinalLayout = ImageLayout::Undefined;

	RenderPassAttachmentDesc() = default;
} RenderPassAttachmentDesc;

typedef struct SubpassDesc {
	PipelineBindPoint BindPoint = PipelineBindPoint::Graphics;
	uint32_t InputAttachmentCount = 0;
	const AttachmentReference* InputAttachments = nullptr;
	uint32_t ColorAttachmentCount = 0;
	const AttachmentReference* ColorAttachments = nullptr;
	const AttachmentReference* ResolveAttachments = nullptr;
	const AttachmentReference* DepthStencilAttachment = nullptr;
	uint32_t PreserveAttachmentCount = 0;
	const uint32_t* PreserveAttachments = nullptr;

	SubpassDesc() = default;
} SubpassDesc;

typedef struct DependencyDesc {
	uint32_t SrcSubpass = 0;
	uint32_t DstSubpass = 0;
	PipelineStage SrcStage = PipelineStage::Undefined;
	PipelineStage DstStage = PipelineStage::Undefined;
	SubpassAccess SrcAccess = SubpassAccess::NA;
	SubpassAccess DstAccess = SubpassAccess::NA;

	DependencyDesc() = default;
} DependencyDesc;

typedef struct RenderPassDesc {
	uint32_t AttachmentCount = 0;
	RenderPassAttachmentDesc* Attachments = nullptr;
	uint32_t SubpassCount = 0;
	SubpassDesc* Subpasses = nullptr;
	uint32_t DependencyCount = 0;
	DependencyDesc* Dependencies = nullptr;

	RenderPassDesc() = default;
} RenderPassDesc;

class GraphicsDevice;
class RenderPass
{
public:
	RenderPass(GraphicsDevice* device, const RenderPassDesc& desc)
		: m_Device(device), m_Desc(desc)
	{}

	const RenderPassDesc& getDesc() { return m_Desc; }
protected:
	GraphicsDevice* m_Device;
	RenderPassDesc m_Desc;
};

