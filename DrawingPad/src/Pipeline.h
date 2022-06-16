#pragma once

#include "RenderPass.h"
#include "Shader.h"
#include "Texture.h"

enum class ElementDataType : uint8_t
{
	None = 0, 
	Float16, 
	Float32,
	Int8,
	Int16,
	Int32,
	Uint8,
	Uint16,
	Uint32,
};

typedef struct LayoutElement {
	uint32_t InputIndex = 0;
	uint32_t BufferSlot = 0;
	uint32_t NumComponents = 0;
	uint32_t Offset = UINT32_MAX;
	uint32_t Stride = UINT32_MAX;
	bool Normalized = false;
	ElementDataType Type = ElementDataType::Float32;
} LayoutElement;

typedef struct InputLayout {
	uint32_t NumElements = 0;
	LayoutElement* Elements = nullptr;
} InputLayout;

typedef struct GraphicsPipelineDesc {
	InputLayout InputLayout;
	uint32_t ShaderCount = 0;
	Shader** Shaders = nullptr;
	uint32_t NumViewports = 0;
	uint8_t NumColors = 0;
	TextureFormat ColorFormats[8];
	TextureFormat DepthFormat;
	uint8_t SampleCount = 1;
	RenderPass* RenderPass = nullptr;
} GraphicsPipelineDesc;

typedef struct ComputePipelineDesc {
	void* Rasterizer = nullptr;
} ComputePipelineDesc;

typedef struct RaytracingPipelineDesc {
	void* Rasterizer = nullptr;
} RaytracingPipelineDesc;

class Pipeline
{
public:
	Pipeline(const GraphicsPipelineDesc& desc)
		: m_GraphicsDesc(desc) {
		m_Type = PipelineBindPoint::Graphics;
	}
	Pipeline(const ComputePipelineDesc& desc)
		: m_ComputeDesc(desc) {
		m_Type = PipelineBindPoint::Compute;
	}
	Pipeline(const RaytracingPipelineDesc& desc)
		: m_RaytracingDesc(desc) {
		m_Type = PipelineBindPoint::Raytracing;
	}

	virtual ~Pipeline() {}

	const PipelineBindPoint& GetBindPoint() const { return m_Type; }
	const GraphicsPipelineDesc& GetGraphicsDesc() const { return m_GraphicsDesc; }
	const ComputePipelineDesc& GetComputeDesc() const { return m_ComputeDesc; }
	const RaytracingPipelineDesc& GetRaytracingDesc() const { return m_RaytracingDesc; }

protected:
	PipelineBindPoint m_Type;
	GraphicsPipelineDesc m_GraphicsDesc = {};
	ComputePipelineDesc m_ComputeDesc = {};
	RaytracingPipelineDesc m_RaytracingDesc = {};
};
