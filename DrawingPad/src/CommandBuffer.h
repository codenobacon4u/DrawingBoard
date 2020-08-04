#pragma once

#include "Buffer.h"
#include "Pipeline.h"
#include "Framebuffer.h"

class CmdList
{
public:
	CmdList() {};

	virtual void Begin() = 0;
	virtual void ClearColor(float* color) = 0;
	virtual void ClearDepth() = 0;
	virtual void Draw() = 0;
	virtual void DrawIndexed(uint32_t indexCount) = 0;
	virtual void DrawIndirect() = 0;
	virtual void DrawIndexedIndirect() = 0;
	virtual void Dispatch() = 0;
	virtual void DispatchIndirect() = 0;
	virtual void End() = 0;
	virtual void SetFramebuffer(Framebuffer* framebuffer) = 0;
	virtual void SetIndexBuffer(Buffer* indexBuffer) = 0;
	virtual void SetVertexBuffer(Buffer* vertexbuffer) = 0;
	virtual void SetPipeline(Pipeline* pipeline) = 0;
};