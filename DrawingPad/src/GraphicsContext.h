#pragma once

#include "Buffer.h"
#include "Texture.h"
#include "RenderPass.h"
#include "Framebuffer.h"
#include "Pipeline.h"

#define MAX_VIEWPORTS 16
#define MAX_RENDER_TARGETS 8

enum class ClearDepthStencil : uint32_t {
	None = 0,
	Depth = 1,
	Stencil = 2
};

struct Float4 {
	float r;
	float g;
	float b;
	float a;
};

struct Rec {
	float X = 0.0f;
	float Y = 0.0f;
	float Width = 0.0f;
	float Height = 0.0f;
};

struct GraphicsContextDesc {
	const char* Name = nullptr;
	bool Deferred = false;
	uint8_t ContextID = 0;
	uint8_t QueueID = 0;
};

struct DrawAttribs {
	uint32_t VrtIdxCount;
	uint32_t InstanceCount;
	uint32_t FirstVrtIdx;
	uint32_t FirstInstance;
	uint32_t VertexOffset;
};

struct DrawIndexAttribs {
	uint32_t IndexCount;
	uint32_t InstanceCount;
	uint32_t FirstIndex;
	uint32_t VertexOffset;
	uint32_t FirstInstance;
};

struct DrawIndirectAttribs {
	Buffer* Buffer;
	uint64_t Offset;
	uint32_t DrawCount;
	uint32_t Stride;
};

struct DispatchAttribs {
	uint32_t GroupCountX;
	uint32_t GroupCountY;
	uint32_t GroupCountZ;
};

struct DispatchIndirectAttribs {
	Buffer* Buffer;
	uint64_t Offset;
};

struct BeginRenderPassAttribs {
	RenderPass* RenderPass;
	Framebuffer* Framebuffer;
	uint32_t ClearCount;
	Float4* ClearValues;
};

struct Viewport {
	float X = 0.0f;
	float Y = 0.0f;
	float Width = 0.0f;
	float Height = 0.0f;
	float minDepth = 0.0f;
	float maxDepth = 1.0f;
};

class GraphicsContext
{
public:
	GraphicsContext(GraphicsDevice* device, const GraphicsContextDesc& desc) : 
		m_Device(device), 
		m_Desc 
		{
			desc.Name != nullptr && *desc.Name != '\0' ? desc.Name : "Context #" + desc.ContextID + desc.Deferred ? " deferred" : " immediate", 
			desc.Deferred,
			desc.ContextID,
			desc.QueueID
		}
	{
	}

	virtual ~GraphicsContext() {}

	virtual const GraphicsContextDesc& GetDesc() const final { return m_Desc; }

	virtual void Flush() = 0;

	virtual void Begin(uint32_t frameIdx) = 0;
	//virtual void End() = 0;
	virtual void BeginRenderPass(const BeginRenderPassAttribs& attribs) = 0;
	virtual void EndRenderPass() = 0;

	virtual void ClearColor(TextureView* tv, const float* color) = 0;
	virtual void ClearDepth(TextureView* tv, ClearDepthStencil clearFlags, float depth, uint8_t stencil) = 0;

	virtual void SetViewports(uint32_t num, const Viewport* viewports, uint32_t width, uint32_t height) = 0;
	virtual void SetScissors(uint32_t num, int32_t offX, int32_t offY, uint32_t width, uint32_t height) = 0;
	virtual void SetRenderTargets(uint32_t numTargets, TextureView** targets, TextureView* depthStencil, bool clear) = 0;
	virtual void SetPipeline(Pipeline* pipeline) = 0;
	virtual void SetVertexBuffers(uint32_t start, uint32_t num, Buffer** buffers, const uint64_t* offsets) = 0;
	virtual void SetIndexBuffer(Buffer* buffer, uint32_t offset) = 0;
	virtual void SetShaderResource(ResourceBindingType type, uint32_t set, uint32_t binding, Buffer* buffer) = 0;
	virtual void SetShaderResource(ResourceBindingType type, uint32_t set, uint32_t binding, Texture* buffer) = 0;
	virtual void SetPushConstant(ShaderType stage, uint32_t offset, uint32_t size, void* data) = 0;
	
	virtual void Draw(const DrawAttribs& attribs) = 0;
	virtual void DrawIndexed(const DrawIndexAttribs& attribs) = 0;
	virtual void DrawIndirect(const DrawIndirectAttribs& attribs) = 0;
	virtual void DrawIndexedIndirect(const DrawIndirectAttribs& attribs) = 0;
	//TODO: Look into drawing meshes
	//virtual void DrawMesh() = 0;
	//virtual void DrawMeshIndirect() = 0;
	//virtual void DrawMeshIndirectCount() = 0;

	virtual void Dispatch(const DispatchAttribs& attribs) = 0;
	virtual void DispatchIndirect(const DispatchIndirectAttribs& attribs) = 0;

	virtual void ResourceBarrier() = 0;
	virtual void UploadBuffer(Buffer* src, uint32_t offset, size_t size, void* data) = 0;
	virtual void CopyBuffer(Buffer* src, Buffer* dst, uint32_t size) = 0;
	virtual void UploadTexture() = 0;

protected:
	GraphicsDevice* m_Device;
	
	uint32_t m_NumVertexBuffers = 0;
	Buffer* m_VertexBuffers[32] = {};
	uint64_t m_VertexOffsets[32] = { 0 };
	Buffer* m_IndexBuffer = nullptr;

	uint32_t m_NumViewports = 0;
	Viewport m_Viewports[MAX_VIEWPORTS];

	uint32_t m_NumScissorRecs = 0;
	Rec m_ScissorRecs[MAX_VIEWPORTS];

	TextureView* m_RenderTargets[MAX_RENDER_TARGETS] = {};
	uint32_t m_NumRenderTargets = 0;

	TextureView* m_DepthStencil = nullptr;

	Pipeline* m_Pipeline = nullptr;

	uint32_t m_FramebufferWidth = 0;
	uint32_t m_FramebufferHeight = 0;
	uint32_t m_FramebufferSlices = 0;
	uint32_t m_FramebufferSamples = 0;

	GraphicsContextDesc m_Desc;
};
