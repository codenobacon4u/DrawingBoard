#pragma once
#include "CommandBuffer.h"
#include "Framebuffer.h"

#include <stdint.h>
#include <list>

struct OpenGLCmdEntry {
	enum class Type {
		Begin,
		ClearColor,
		ClearDepth,
		Draw,
		DrawIndexed,
		DrawIndirect,
		DrawIndexedIndirect,
		Dispatch,
		DispatchIndirect,
		End,
		SetFramebuffer,
		SetIndexBuffer,
		SetVertexBuffer,
		SetPipeline,
		SetGraphicsResource,
		SetComputeResource,
		SetViewport,
		UpdateBuffer,
		CopyBuffer,
		UpdateTexture,
		CopyTexture,

	};
	Type CmdType;
	void* CmdData;

	OpenGLCmdEntry(Type type) {
		CmdType = type;
	}

	OpenGLCmdEntry(Type type, void* data) {
		CmdType = type;
		CmdData = data;
	}
};

class OpenGLCmdEntryList
{
public:
	OpenGLCmdEntryList() {}
	void Begin();
	void ClearColor(float* clearColor);
	void ClearDepth();
	void DrawIndexed(uint32_t indexCount);
	void End();
	void SetFramebuffer(Framebuffer* framebuffer);
	void SetIndexBuffer(Buffer* indexBuffer);
	void SetVertexBuffer(Buffer* vertexBuffer);
	void SetPipeline(Pipeline* pipeline);

	std::list<OpenGLCmdEntry> Get() { return m_Commands; }

private:
	std::list<OpenGLCmdEntry> m_Commands;

};

class OpenGLCmdList : public CmdList 
{
public:
	OpenGLCmdList() {}

	virtual void Begin();
	virtual void ClearColor(float* clearColor);
	virtual void ClearDepth();
	virtual void Draw();
	virtual void DrawIndexed(uint32_t indexCount);
	virtual void DrawIndirect();
	virtual void DrawIndexedIndirect();
	virtual void Dispatch();
	virtual void DispatchIndirect();
	virtual void End();
	virtual void SetFramebuffer(Framebuffer* framebuffer);
	virtual void SetIndexBuffer(Buffer* indexBuffer);
	virtual void SetVertexBuffer(Buffer* vertexBuffer);
	virtual void SetPipeline(Pipeline* pipeline);

	OpenGLCmdEntryList* GetCommands() { return m_Current; }
private:
	OpenGLCmdEntryList* m_Current;
};