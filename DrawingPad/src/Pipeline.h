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

enum class FrontFace : uint8_t
{
	CounterClockwise = 0,
	Clockwise = 1
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

typedef struct GraphicsPipelineDesc {
	const std::vector<LayoutElement> InputLayout;
	ShaderProgram* ShaderProgram = nullptr;
	uint32_t NumViewports = 0;
	bool DepthEnable = true;
	FrontFace Face = FrontFace::CounterClockwise;
	uint8_t MSAASamples = 0;
	RenderPass* RenderPass = nullptr;
} GraphicsPipelineDesc;

typedef struct ComputePipelineDesc {
	ShaderProgram* ShaderProgram = nullptr;
} ComputePipelineDesc;

typedef struct RaytracingPipelineDesc {
	ShaderProgram* ShaderProgram = nullptr;
	uint32_t MaxRecursion = 0;
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

protected:
	PipelineBindPoint m_Type;
	GraphicsPipelineDesc m_GraphicsDesc = {};
	ComputePipelineDesc m_ComputeDesc = {};
	RaytracingPipelineDesc m_RaytracingDesc = {};
};
