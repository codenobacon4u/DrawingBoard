#include <stdio.h>
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <DrawingPad.h>
#include <fstream>
#include <chrono>
#include <iostream>
#include <glm/glm/gtx/string_cast.hpp>
#include <glm/glm/gtx/rotate_vector.hpp>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#define TINYOBJLOADER_IMPLEMENTATION
#include <tiny_obj_loader.h>

#include "ImGuiWindow.h"
#include "ImGuiRenderer.h"

struct Vertex {
	glm::vec3 pos;
	glm::vec3 color;
	glm::vec2 tex;

	bool operator==(const Vertex& other) const {
		return pos == other.pos && color == other.color && tex == other.tex;
	}
};

struct UniformBufferObject {
	alignas(16) glm::mat4 model;
	alignas(16) glm::mat4 view;
	alignas(16) glm::mat4 proj;
};

API Curr_API = API::Vulkan;

static Swapchain* gSwapchain;
static Buffer* gVertexBuffer;
static Buffer* gIndexBuffer;
static Buffer* gUniformBuffer;
static Pipeline* gPipeline;

static Texture* gTexture;
static Texture* renderTexture;

static std::vector<Vertex> gVertices;
static std::vector<uint32_t> gIndices;

static void glfw_error_callback(int error, const char* desc) {
	std::cerr << "GLFW Error: [" << error << "] " << desc;
}

void RenderScene(CommandBuffer* cmd, TextureView* rtv) {
	float width = static_cast<float>(rtv->GetTexture()->GetDesc().Width);
	float height = static_cast<float>(rtv->GetTexture()->GetDesc().Height);
	cmd->SetViewports(0, 1, { { 0, 0, width, height, 0.0f, 1.0f } });
	cmd->SetScissors(0, 1, { { { 0, 0 }, { width, height } } });
	cmd->BindPipeline(gPipeline);
	cmd->BindVertexBuffer(0, 1, { gVertexBuffer }, { 0 });
	cmd->BindIndexBuffer(gIndexBuffer, 0, 1);

	{
		static auto startTime = std::chrono::high_resolution_clock::now();
		auto currentTime = std::chrono::high_resolution_clock::now();
		float time = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();
		UniformBufferObject ubo = {};
		ubo.model = glm::rotate(glm::mat4(1.0f), glm::radians(0.f), glm::vec3(0.0f, 0.0f, 1.0f));
		ubo.view = glm::lookAt(glm::vec3(2.0f, 2.0f, 2.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
		ubo.proj = glm::perspective(glm::radians(45.0f), 16.0f / 9.0f, 0.1f, 10.0f);
		ubo.proj[1][1] *= -1;

		void* data;
		gUniformBuffer->MapMemory(gSwapchain->GetImageIndex() * sizeof(ubo), sizeof(ubo), &data);
		memcpy(data, &ubo, sizeof(ubo));
		gUniformBuffer->FlushMemory();

		cmd->BindBuffer(gUniformBuffer, gSwapchain->GetImageIndex() * sizeof(ubo), sizeof(ubo), 0, 0);
	}

	cmd->BindImage(gTexture, 0, 1);
	cmd->DrawIndexed(static_cast<uint32_t>(gIndices.size()), 1, 0, 0, 1);
}

void FrameRender(ImGui_ImplDrawingPad_Window* windowData, ImDrawData* drawData)
{
	// Acquire Next Image
	auto cmd = windowData->Context->Begin();
	auto rtv = windowData->Swapchain->GetBackbuffer();
	auto dsv = windowData->Swapchain->GetDepthBufferView();

	// Begin Command Buffer
	cmd->Begin();
	cmd->BeginRenderPass(windowData->RenderPass, { rtv, dsv }, { windowData->ClearValue, { 1.0f, 0 } });
	
	RenderScene(cmd, rtv);

	// Record Dear ImGui Primitives Into CommandBuffer
	ImGui_ImplDrawingPad_RenderDrawData(drawData, cmd);

	// Submit CommandBuffer
	cmd->EndRenderPass();
	cmd->End();
	windowData->Context->Submit({ cmd });
}

void FramePresent(ImGui_ImplDrawingPad_Window* windowData)
{
	windowData->Context->Present();
}

static ImGui_ImplDrawingPad_Window g_MainWindowData;

int main() {
	remove("validation_layers.log");
	glfwSetErrorCallback(glfw_error_callback);

	if (!glfwInit())
		printf("Failed");
	if (Curr_API != API::OpenGL)
		glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	GLFWwindow* window = glfwCreateWindow(1280, 720, "DrawingPad Editor", NULL, NULL);
	GraphicsDevice* device = GraphicsDevice::Create(window, Curr_API);

	int w, h;
	glfwGetFramebufferSize(window, &w, &h);
	ImGui_ImplDrawingPad_Window* wd = &g_MainWindowData;

	// Create Framebuffers
	{
		// CreateWindowSwapChain
		SwapchainDesc swapDesc = {};
		swapDesc.Width = w;
		swapDesc.Height = h;
		swapDesc.SurfaceFormats = { TextureFormat::BGRA8Unorm, TextureFormat::RGBA8Unorm, TextureFormat::BGR8Unorm, TextureFormat::RGB8Unorm };
		//swapDesc.DepthFormat = TextureFormat::None;
		wd->Width = w;
		wd->Height = h;
		wd->Swapchain = device->CreateSwapchain(swapDesc, window);
		wd->Context = device->CreateGraphicsContext(wd->Swapchain);

		RenderPassDesc rpDesc = {};
		std::vector<RenderPassAttachmentDesc> attachments = {
			{
				wd->Swapchain->GetDesc().ColorFormat,
				1,
				AttachmentLoadOp::Clear,
				AttachmentStoreOp::Store,
				AttachmentLoadOp::Discard,
				AttachmentStoreOp::Discard,
				ImageLayout::Undefined,
				ImageLayout::PresentSrcKHR
			},
			{
				wd->Swapchain->GetDesc().DepthFormat,
				1,
				AttachmentLoadOp::Clear,
				AttachmentStoreOp::DontCare,
				AttachmentLoadOp::DontCare,
				AttachmentStoreOp::DontCare,
				ImageLayout::Undefined,
				ImageLayout::DepthStencilAttachOptimal
			},
		}; 

		AttachmentReference depthAttach = { 1, ImageLayout::DepthStencilAttachOptimal };
		SubpassDesc subpass = {};
		subpass.BindPoint = PipelineBindPoint::Graphics;
		subpass.ColorAttachments = { { 0, ImageLayout::ColorAttachOptimal } };
		subpass.DepthStencilAttachment = &depthAttach;
		
		DependencyDesc dependency = {};
		dependency.SrcSubpass = ~0U;
		dependency.DstSubpass = 0;
		dependency.SrcStage = PipelineStage::ColorAttachOutput;
		dependency.DstStage = PipelineStage::ColorAttachOutput;
		dependency.SrcAccess = SubpassAccess::NA;
		dependency.DstAccess = SubpassAccess::ColorAttachWrite;
		
		rpDesc.Attachments = attachments;
		rpDesc.Subpasses = { subpass };
		rpDesc.SubpassDependencies = { dependency };
		wd->RenderPass = device->CreateRenderPass(rpDesc);
	}

	// Create Scene Objects
	{

		{
			tinyobj::attrib_t attrib;
			std::vector<tinyobj::shape_t> shapes;
			std::vector<tinyobj::material_t> materials;
			std::string warn, err;

			if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, "models/viking_room.obj")) {
				throw std::runtime_error(warn + err);
			}

			std::unordered_map<size_t, uint32_t> uniqueVertices{};

			for (const auto& shape : shapes) {
				for (const auto& index : shape.mesh.indices) {
					Vertex vertex{};

					vertex.pos = {
						attrib.vertices[3 * index.vertex_index + 0],
						attrib.vertices[3 * index.vertex_index + 1],
						attrib.vertices[3 * index.vertex_index + 2]
					};

					vertex.tex = {
						attrib.texcoords[2 * index.texcoord_index + 0],
						1.0f - attrib.texcoords[2 * index.texcoord_index + 1]
					};

					vertex.color = { 1.0f, 1.0f, 1.0f };

					size_t hash = 0;
					hash_combine(hash, vertex.pos);
					hash_combine(hash, vertex.color);
					hash_combine(hash, vertex.tex);

					if (uniqueVertices.count(hash) == 0) {
						uniqueVertices[hash] = static_cast<uint32_t>(gVertices.size());
						gVertices.push_back(vertex);
					}

					gIndices.push_back(uniqueVertices[hash]);
				}
			}
		}

		BufferDesc bufDesc = {};
		bufDesc.Usage = BufferUsageFlags::Default;
		// ======== Create Vertex Buffer ========
		bufDesc.BindFlags = BufferBindFlags::Vertex;
		bufDesc.Size = static_cast<uint32_t>(sizeof(gVertices[0]) * gVertices.size());
		gVertexBuffer = device->CreateBuffer(bufDesc, (void*)gVertices.data());

		// ======== Create Index Buffer ========
		bufDesc.BindFlags = BufferBindFlags::Index;
		bufDesc.Size = static_cast<uint32_t>(sizeof(gIndices[0]) * gIndices.size());
		gIndexBuffer = device->CreateBuffer(bufDesc, (void*)gIndices.data());

		// ======== Create Uniform Buffer ========
		bufDesc.BindFlags = BufferBindFlags::Uniform;
		bufDesc.Size = static_cast<uint32_t>(sizeof(UniformBufferObject) * 3);
		gUniformBuffer = device->CreateBuffer(bufDesc, nullptr);

		int width, height, channels;
		stbi_uc* pixels = stbi_load("textures/viking_room.png", &width, &height, &channels, STBI_rgb_alpha);
		TextureDesc texDesc = {};
		texDesc.Type = TextureType::DimTex2D;
		texDesc.Width = static_cast<uint32_t>(width);
		texDesc.Height = static_cast<uint32_t>(height);
		texDesc.MipLevels = static_cast<uint32_t>(std::floor(std::log2(std::max(width, height)))) + 1;
		texDesc.Format = TextureFormat::RGBA8UnormSRGB;
		texDesc.ArraySize = 1;
		texDesc.BindFlags = BindFlags::ShaderResource;
		gTexture = device->CreateTexture(texDesc, pixels);

		// ======== Create Shaders ========
		ShaderDesc sDesc = {};
		sDesc.EntryPoint = "main";
		sDesc.Name = "Basic Vert";
		//sDesc.Src = vertSrc;
		sDesc.Path = "shaders/ubo.vert";
		sDesc.Type = ShaderType::Vertex;
		auto* vertShader = device->CreateShader(sDesc);
		sDesc.Name = "Basic Frag";
		//sDesc.Src = fragSrc;
		sDesc.Path = "shaders/ubo.frag";
		sDesc.Type = ShaderType::Fragment;
		auto* fragShader = device->CreateShader(sDesc);

		LayoutElement vertInputs[]{
			{
				0, // InputIndex Location
				0, // BufferSlot Binding
				3, // Num Components
				offsetof(Vertex, pos), // Offset
				sizeof(Vertex) // Stride
			},
			{
				1, // InputIndex Location
				0, // BufferSlot Binding
				3, // Num Components
				offsetof(Vertex, color),  // Offset
				sizeof(Vertex) // Stride
			}
			,{
				2,
				0,
				2,
				offsetof(Vertex, tex),
				sizeof(Vertex)
			}
		};

		std::vector<Shader*> shaders = { vertShader, fragShader };

		GraphicsPipelineDesc pDesc = {};
		pDesc.NumViewports = 1;
		pDesc.NumColors = 1;
		pDesc.ColorFormats[0] = wd->Swapchain->GetDesc().ColorFormat;
		pDesc.DepthFormat = wd->Swapchain->GetDesc().DepthFormat;
		pDesc.InputLayout.NumElements = 3;
		pDesc.InputLayout.Elements = vertInputs;
		pDesc.ShaderCount = 2;
		pDesc.Shaders = shaders.data();
		pDesc.Face = FrontFace::CounterClockwise;
		//pDesc.MSAASamples = 4;
		gPipeline = device->CreateGraphicsPipeline(pDesc, wd->RenderPass);

		gSwapchain = wd->Swapchain;
	}

	// Setup Dear ImGui Context
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO(); (void)io;
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;       // Enable Keyboard Controls
	//io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls
	io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;           // Enable Docking
	io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;         // Enable Multi-Viewport / Platform Windows
	//io.ConfigViewportsNoAutoMerge = true;
	//io.ConfigViewportsNoTaskBarIcon = true;

	// Setup Dear ImGui style
	ImGui::StyleColorsDark();
	//ImGui::StyleColorsClassic();

	// When viewports are enabled we tweak WindowRounding/WindowBg so platform windows can look identical to regular ones.
	ImGuiStyle& style = ImGui::GetStyle();
	if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
	{
		style.WindowRounding = 0.0f;
		style.Colors[ImGuiCol_WindowBg].w = 1.0f;
	}

	// Setup Platform/Renderer backends
	ImGui_ImplGlfw_InitForOther(window, true);
	//	[Device, PipelineCache, Descriptors
	ImGui_ImplDrawingPad_Init(device, wd->RenderPass, wd->Swapchain->GetDesc().BufferCount);

	// Upload Fonts
	{
		// Create Fonts Texture
		ImGui_ImplDrawingPad_CreateFontsTexture();
	}

	// Our state
	bool show_demo_window = true;
	bool show_another_window = false;
	ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);

	// Main Loop
	while (!glfwWindowShouldClose(window)) 
	{
		glfwPollEvents();

		// Resize SwapChain?

		// Start the Dear ImGui frame
		// ImplDrawingPad_NewFrame();
		ImGui_ImplGlfw_NewFrame();
		ImGui::NewFrame();

		// 1. Show the big demo window (Most of the sample code is in ImGui::ShowDemoWindow()! You can browse its code to learn more about Dear ImGui!).
		if (show_demo_window)
			ImGui::ShowDemoWindow(&show_demo_window);

		// 2. Show a simple window that we create ourselves. We use a Begin/End pair to created a named window.
		{
			static float f = 0.0f;
			static int counter = 0;

			ImGui::Begin("Hello, world!");                          // Create a window called "Hello, world!" and append into it.

			ImGui::Text("This is some useful text.");               // Display some text (you can use a format strings too)
			ImGui::Checkbox("Demo Window", &show_demo_window);      // Edit bools storing our window open/close state
			ImGui::Checkbox("Another Window", &show_another_window);

			ImGui::SliderFloat("float", &f, 0.0f, 1.0f);            // Edit 1 float using a slider from 0.0f to 1.0f
			ImGui::ColorEdit3("clear color", (float*)&clear_color); // Edit 3 floats representing a color

			if (ImGui::Button("Button"))                            // Buttons return true when clicked (most widgets return true when edited/activated)
				counter++;
			ImGui::SameLine();
			ImGui::Text("counter = %d", counter);

			ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
			ImGui::End();
		}

		static int resWidth = 1280;
		static int resHeight = 720;
		// 3. Show another simple window.
		if (show_another_window)
		{
			ImGui::Begin("Game Window Settings", &show_another_window);   // Pass a pointer to our bool variable (the window will have a closing button that will clear the bool when clicked)
			resWidth = static_cast<int>((16.0f / 9.0f) * resHeight);
			resHeight = static_cast<int>((9.0f / 16.0f) * resWidth);
			ImGui::SliderInt("Resolution Width: ", &resWidth, 16, 4096);
			ImGui::SliderInt("Resolution Height: ", &resHeight, 9, 2304);
			ImGui::End();
		}



		// Rendering
		ImGui::Render();
		ImDrawData* main_draw_data = ImGui::GetDrawData();
		const bool main_is_minimized = (main_draw_data->DisplaySize.x <= 0.0f || main_draw_data->DisplaySize.y <= 0.0f);
		wd->ClearValue.color[0] = clear_color.x * clear_color.w;
		wd->ClearValue.color[1] = clear_color.y * clear_color.w;
		wd->ClearValue.color[2] = clear_color.z * clear_color.w;
		wd->ClearValue.color[3] = clear_color.w;
		if (!main_is_minimized)
			FrameRender(wd, main_draw_data);

		// Update and Render additional Platform Windows
		if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
		{
			ImGui::UpdatePlatformWindows();
			ImGui::RenderPlatformWindowsDefault();
		}

		// Present Main Platform Window
		if (!main_is_minimized)
			FramePresent(wd);
	}
	device->WaitForIdle();
	ImGui_ImplDrawingPad_Shutdown();
	ImGui_ImplGlfw_Shutdown();
	ImGui::DestroyContext();

	glfwDestroyWindow(window);
	glfwTerminate();

	return 0;
}
