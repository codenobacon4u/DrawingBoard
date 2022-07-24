#pragma once

#include <glm/glm.hpp>
#include <DrawingPad.h>
#include <imgui.h>

struct ImGui_ImplDrawingPad_Window
{
	int Width;
	int Height;
	GraphicsContext* Context;
	Pipeline* Pipeline;
	RenderPass* RenderPass;
	Swapchain* Swapchain;
	bool ClearEnable;
	ClearValue ClearValue;

	ImGui_ImplDrawingPad_Window() {
		memset(this, 0, sizeof(*this));
		ClearEnable = true;
	}
};

struct ImGui_ImplDrawingPad_FrameRenderBuffers 
{
	Buffer* VertexBuffer = nullptr;
	Buffer* IndexBuffer = nullptr;
	Buffer* UploadBuffer = nullptr;
};

struct ImGui_ImplDrawingPad_ViewportData
{
	bool WindowOwned = false;
	ImGui_ImplDrawingPad_Window Window;
	uint32_t Index = 0;
	std::vector<ImGui_ImplDrawingPad_FrameRenderBuffers> RenderBuffers = {};
};

struct ImGui_ImplDrawingPad_Data
{
	GraphicsDevice* Device;
	RenderPass* RenderPass;
	uint64_t BufferMemoryAlignment;
	Pipeline* Pipeline;
	uint32_t Subpass;
	ShaderProgram* ShaderProgram;
	Shader* VertexShader;
	Shader* FragmentShader;

	Texture* FontTexture;
	Buffer* UploadBuffer;

	uint32_t ImageCount;
	std::vector<ImGui_ImplDrawingPad_FrameRenderBuffers> MainWindowBuffers;

	ImGui_ImplDrawingPad_Data()
	{
		memset((void*)this, 0, sizeof(*this));
		BufferMemoryAlignment = 256;
	}
};

void ImGui_ImplDrawingPad_Init(GraphicsDevice* device, RenderPass* renderpass, uint32_t imageCount);
void ImGui_ImplDrawingPad_CreateFontsTexture();
void ImGui_ImplDrawingPad_RenderDrawData(ImDrawData* drawData, CommandBuffer* cmd, Pipeline* pipeline = nullptr);
void ImGui_ImplDrawingPad_Shutdown();

void ImGui_ImplDrawingPad_CreateOrResizeWindow();
