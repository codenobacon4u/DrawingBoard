#pragma once

#include <glm/glm.hpp>
#include <DrawingPad.h>
#include <imgui.h>

struct ImGui_ImplDrawingPad_Window
{
	int Width;
	int Height;
	DrawingPad::GraphicsContext* Context;
	DrawingPad::Pipeline* Pipeline;
	DrawingPad::RenderPass* RenderPass;
	DrawingPad::Swapchain* Swapchain;
	bool ClearEnable;
	DrawingPad::ClearValue ClearValue;

	ImGui_ImplDrawingPad_Window() {
		memset(this, 0, sizeof(*this));
		ClearEnable = true;
	}
};

struct ImGui_ImplDrawingPad_FrameRenderBuffers 
{
	DrawingPad::Buffer* VertexBuffer = nullptr;
	DrawingPad::Buffer* IndexBuffer = nullptr;
	DrawingPad::Buffer* UploadBuffer = nullptr;
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
	DrawingPad::GraphicsDevice* Device;
	DrawingPad::RenderPass* RenderPass;
	uint64_t BufferMemoryAlignment;
	DrawingPad::Pipeline* Pipeline;
	uint32_t Subpass;
	DrawingPad::ShaderProgram* ShaderProgram;
	DrawingPad::Shader* VertexShader;
	DrawingPad::Shader* FragmentShader;

	DrawingPad::Texture* FontTexture;
	DrawingPad::Buffer* UploadBuffer;

	uint32_t ImageCount;
	std::vector<ImGui_ImplDrawingPad_FrameRenderBuffers> MainWindowBuffers;

	ImGui_ImplDrawingPad_Data()
	{
		memset((void*)this, 0, sizeof(*this));
		BufferMemoryAlignment = 256;
	}
};

void ImGui_ImplDrawingPad_Init(DrawingPad::GraphicsDevice* device, DrawingPad::RenderPass* renderpass, uint32_t imageCount);
void ImGui_ImplDrawingPad_CreateFontsTexture();
void ImGui_ImplDrawingPad_RenderDrawData(ImDrawData* drawData, DrawingPad::CommandBuffer* cmd, DrawingPad::Pipeline* pipeline = nullptr);
void ImGui_ImplDrawingPad_Shutdown();

void ImGui_ImplDrawingPad_CreateOrResizeWindow();
