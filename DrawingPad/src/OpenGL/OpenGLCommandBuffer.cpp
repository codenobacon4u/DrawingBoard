#include "pwpch.h"
#include "OpenGLCommandBuffer.h"

void OpenGLCmdList::Begin()
{
	m_Current->Begin();
}

void OpenGLCmdList::ClearColor(float* clearColor)
{
	m_Current->ClearColor(clearColor);
}

void OpenGLCmdList::ClearDepth()
{
	m_Current->ClearDepth();
}

void OpenGLCmdList::Draw()
{
}

void OpenGLCmdList::DrawIndexed(uint32_t indexCount)
{
	m_Current->DrawIndexed(indexCount);
}

void OpenGLCmdList::DrawIndirect()
{
}

void OpenGLCmdList::DrawIndexedIndirect()
{
}

void OpenGLCmdList::Dispatch()
{
}

void OpenGLCmdList::DispatchIndirect()
{
}

void OpenGLCmdList::End()
{
	m_Current->End();
}

void OpenGLCmdList::SetFramebuffer(Framebuffer* framebuffer)
{
	m_Current->SetFramebuffer(framebuffer);
}

void OpenGLCmdList::SetIndexBuffer(Buffer* indexBuffer)
{
	m_Current->SetIndexBuffer(indexBuffer);
}

void OpenGLCmdList::SetVertexBuffer(Buffer* vertexBuffer)
{
	m_Current->SetVertexBuffer(vertexBuffer);
}

void OpenGLCmdList::SetPipeline(Pipeline* pipeline)
{
	m_Current->SetPipeline(pipeline);
}

void OpenGLCmdEntryList::Begin()
{
	m_Commands.push_back(OpenGLCmdEntry(OpenGLCmdEntry::Type::Begin));
}

void OpenGLCmdEntryList::ClearColor(float* clearColor)
{
	m_Commands.push_back(OpenGLCmdEntry(OpenGLCmdEntry::Type::ClearColor, clearColor));
}

void OpenGLCmdEntryList::DrawIndexed(uint32_t indexCount)
{
	m_Commands.push_back(OpenGLCmdEntry(OpenGLCmdEntry::Type::DrawIndexed, (void*)indexCount));
}

void OpenGLCmdEntryList::End()
{
	m_Commands.push_back(OpenGLCmdEntry(OpenGLCmdEntry::Type::End));
}

void OpenGLCmdEntryList::SetFramebuffer(Framebuffer* framebuffer)
{
	m_Commands.push_back(OpenGLCmdEntry(OpenGLCmdEntry::Type::SetFramebuffer, framebuffer));
}

void OpenGLCmdEntryList::SetIndexBuffer(Buffer* indexBuffer)
{
	m_Commands.push_back(OpenGLCmdEntry(OpenGLCmdEntry::Type::SetIndexBuffer, indexBuffer));
}

void OpenGLCmdEntryList::SetVertexBuffer(Buffer* vertexBuffer)
{
	m_Commands.push_back(OpenGLCmdEntry(OpenGLCmdEntry::Type::SetVertexBuffer, vertexBuffer));
}

void OpenGLCmdEntryList::SetPipeline(Pipeline* pipeline)
{
	m_Commands.push_back(OpenGLCmdEntry(OpenGLCmdEntry::Type::SetPipeline, pipeline));
}
