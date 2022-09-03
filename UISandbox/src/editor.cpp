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

DrawingPad::API Curr_API = DrawingPad::API::Vulkan;

static DrawingPad::Swapchain* gSwapchain;
static DrawingPad::Buffer* gVertexBuffer;
static DrawingPad::Buffer* gIndexBuffer;
static DrawingPad::Buffer* gUniformBuffer;
static DrawingPad::Pipeline* gPipeline;

static DrawingPad::Texture* gTexture;
static DrawingPad::Texture* renderTexture;

static std::vector<Vertex> gVertices;
static std::vector<uint32_t> gIndices;

static DrawingPad::Buffer* gGameUniformBuffer;
static DrawingPad::Texture* gGameColorTexture;
static DrawingPad::Texture* gGameDepthTexture;
static uint32_t gGameWidth = 1280;
static uint32_t gGameHeight = 720;
static DrawingPad::RenderPass* gRenderPass;
static DrawingPad::Pipeline* gPipeline2;

static bool gameViewable = true;

static std::vector<std::function<void(DrawingPad::CommandBuffer*)>> deferred = {};

void Defer(std::function<void(DrawingPad::CommandBuffer*)> func) {
	deferred.push_back(func);
}

void ProcessDeferred(DrawingPad::CommandBuffer* cmd) {
	for (auto& func : deferred)
		func(cmd);
}

static void glfw_error_callback(int error, const char* desc) {
	std::cerr << "GLFW Error: [" << error << "] " << desc;
}

void RenderGame(DrawingPad::CommandBuffer* cmd, DrawingPad::TextureView* rtv)
{
	cmd->SetViewports(0, 1, { { 0, 0, (float)gGameWidth, (float)gGameHeight, 0.0f, 1.0f } });
	cmd->SetScissors(0, 1, { { { 0, 0 }, { gGameWidth, gGameHeight } } });
	cmd->BindPipeline(gPipeline2);
	cmd->BindVertexBuffer(0, 1, { gVertexBuffer }, { 0 });
	cmd->BindIndexBuffer(gIndexBuffer, 0, 1);

	{
		static auto startTime = std::chrono::high_resolution_clock::now();
		auto currentTime = std::chrono::high_resolution_clock::now();
		float time = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();
		UniformBufferObject ubo = {};
		ubo.model = glm::rotate(glm::mat4(1.0f), time * glm::radians(90.f), glm::vec3(0.0f, 0.0f, 1.0f));
		ubo.view = glm::lookAt(glm::vec3(2.0f, 2.0f, 2.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
		ubo.proj = glm::perspective(glm::radians(45.0f), (float)gGameWidth / (float)gGameHeight, 0.1f, 10.0f);
		ubo.proj[1][1] *= -1;

		gGameUniformBuffer->Update(gSwapchain->GetImageIndex() * sizeof(ubo), sizeof(ubo), reinterpret_cast<uint8_t*>(&ubo));

		cmd->BindBuffer(gGameUniformBuffer, gSwapchain->GetImageIndex() * sizeof(ubo), sizeof(ubo), 0, 0);
	}

	cmd->BindImage(gTexture, 0, 1);
	cmd->DrawIndexed(static_cast<uint32_t>(gIndices.size()), 1, 0, 0, 1);
}

void RenderScene(DrawingPad::CommandBuffer* cmd, DrawingPad::TextureView* rtv) {
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

		gUniformBuffer->Update(gSwapchain->GetImageIndex() * sizeof(ubo), sizeof(ubo), &ubo);

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

	if (true) {
		Defer([](DrawingPad::CommandBuffer* cmd) {
			// Record Game pass
			cmd->BeginRenderPass(gRenderPass, { gGameColorTexture->GetDefaultView(), gGameDepthTexture->GetDefaultView() }, { {0.0f, 0.0f, 0.0f, 1.0f}, {1.0f, 0} });
			RenderGame(cmd, gGameColorTexture->GetDefaultView());
			cmd->EndRenderPass();
		});
	}
	ProcessDeferred(cmd);
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
	if (Curr_API != DrawingPad::API::OpenGL)
		glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	GLFWwindow* window = glfwCreateWindow(1280, 720, "DrawingPad Editor", NULL, NULL);
	DrawingPad::GraphicsDevice* device = DrawingPad::GraphicsDevice::Create(window, Curr_API);

	int w, h;
	glfwGetFramebufferSize(window, &w, &h);
	ImGui_ImplDrawingPad_Window* wd = &g_MainWindowData;

	// Create Framebuffers
	{
		// CreateWindowSwapChain
		DrawingPad::SwapchainDesc swapDesc = {};
		swapDesc.Width = w;
		swapDesc.Height = h;
		swapDesc.SurfaceFormats = { DrawingPad::TextureFormat::BGRA8Unorm, DrawingPad::TextureFormat::RGBA8Unorm, DrawingPad::TextureFormat::BGR8Unorm, DrawingPad::TextureFormat::RGB8Unorm };
		//swapDesc.DepthFormat = TextureFormat::None;
		wd->Width = w;
		wd->Height = h;
		wd->Swapchain = device->CreateSwapchain(swapDesc, window);
		wd->Context = device->CreateGraphicsContext(wd->Swapchain);

		{
			DrawingPad::RenderPassDesc rpDesc = {};
			std::vector<DrawingPad::RenderPassAttachmentDesc> attachments = {
				{
					wd->Swapchain->GetDesc().ColorFormat,
					1,
					DrawingPad::AttachmentLoadOp::Clear,
					DrawingPad::AttachmentStoreOp::Store,
					DrawingPad::AttachmentLoadOp::Discard,
					DrawingPad::AttachmentStoreOp::Discard,
					DrawingPad::ImageLayout::Undefined,
					DrawingPad::ImageLayout::PresentSrcKHR
				},
				{
					wd->Swapchain->GetDesc().DepthFormat,
					1,
					DrawingPad::AttachmentLoadOp::Clear,
					DrawingPad::AttachmentStoreOp::DontCare,
					DrawingPad::AttachmentLoadOp::DontCare,
					DrawingPad::AttachmentStoreOp::DontCare,
					DrawingPad::ImageLayout::Undefined,
					DrawingPad::ImageLayout::DepthStencilAttachOptimal
				},
			};

			DrawingPad::AttachmentReference depthAttach = { 1, DrawingPad::ImageLayout::DepthStencilAttachOptimal };
			DrawingPad::SubpassDesc subpass = {};
			subpass.BindPoint = DrawingPad::PipelineBindPoint::Graphics;
			subpass.ColorAttachments = { { 0, DrawingPad::ImageLayout::ColorAttachOptimal } };
			subpass.DepthStencilAttachment = &depthAttach;

			DrawingPad::DependencyDesc dependency = {};
			dependency.SrcSubpass = ~0U;
			dependency.DstSubpass = 0;
			dependency.SrcStage = DrawingPad::PipelineStage::ColorAttachOutput;
			dependency.DstStage = DrawingPad::PipelineStage::ColorAttachOutput;
			dependency.SrcAccess = DrawingPad::SubpassAccess::NA;
			dependency.DstAccess = DrawingPad::SubpassAccess::ColorAttachWrite;

			rpDesc.Attachments = attachments;
			rpDesc.Subpasses = { subpass };
			rpDesc.SubpassDependencies = { dependency };
			wd->RenderPass = device->CreateRenderPass(rpDesc);
		}
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
					DrawingPad::hash_combine(hash, vertex.pos);
					DrawingPad::hash_combine(hash, vertex.color);
					DrawingPad::hash_combine(hash, vertex.tex);

					if (uniqueVertices.count(hash) == 0) {
						uniqueVertices[hash] = static_cast<uint32_t>(gVertices.size());
						gVertices.push_back(vertex);
					}

					gIndices.push_back(uniqueVertices[hash]);
				}
			}
		}

		{
			DrawingPad::RenderPassDesc rpDesc = {};
			std::vector<DrawingPad::RenderPassAttachmentDesc> attachments = {
				{
					DrawingPad::TextureFormat::RGBA8Unorm,
					1,
					DrawingPad::AttachmentLoadOp::Clear,
					DrawingPad::AttachmentStoreOp::Store,
					DrawingPad::AttachmentLoadOp::DontCare,
					DrawingPad::AttachmentStoreOp::DontCare,
					DrawingPad::ImageLayout::Undefined,
					DrawingPad::ImageLayout::ShaderReadOnlyOptimal
				},
				{
					wd->Swapchain->GetDesc().DepthFormat,
					1,
					DrawingPad::AttachmentLoadOp::Clear,
					DrawingPad::AttachmentStoreOp::DontCare,
					DrawingPad::AttachmentLoadOp::DontCare,
					DrawingPad::AttachmentStoreOp::DontCare,
					DrawingPad::ImageLayout::Undefined,
					DrawingPad::ImageLayout::DepthStencilAttachOptimal
				},
			};

			DrawingPad::AttachmentReference depthAttach = { 1, DrawingPad::ImageLayout::DepthStencilAttachOptimal };
			DrawingPad::SubpassDesc subpass = {};
			subpass.BindPoint = DrawingPad::PipelineBindPoint::Graphics;
			subpass.ColorAttachments = { { 0, DrawingPad::ImageLayout::ColorAttachOptimal } };
			subpass.DepthStencilAttachment = &depthAttach;

			std::vector<DrawingPad::DependencyDesc> dependencies = {
				{
					~0U,
					0,
					DrawingPad::PipelineStage::FragmentShader,
					DrawingPad::PipelineStage::ColorAttachOutput,
					DrawingPad::SubpassAccess::ShaderRead,
					DrawingPad::SubpassAccess::ColorAttachWrite
				},
				{
					0,
					~0U,
					DrawingPad::PipelineStage::ColorAttachOutput,
					DrawingPad::PipelineStage::FragmentShader,
					DrawingPad::SubpassAccess::ColorAttachWrite,
					DrawingPad::SubpassAccess::ShaderRead
				}
			};

			rpDesc.Attachments = attachments;
			rpDesc.Subpasses = { subpass };
			rpDesc.SubpassDependencies = dependencies;
			gRenderPass = device->CreateRenderPass(rpDesc);
		}

		DrawingPad::BufferDesc bufDesc = {};
		bufDesc.Usage = DrawingPad::BufferUsageFlags::Default;
		// ======== Create Vertex Buffer ========
		bufDesc.BindFlags = DrawingPad::BufferBindFlags::Vertex;
		bufDesc.Size = static_cast<uint32_t>(sizeof(gVertices[0]) * gVertices.size());
		gVertexBuffer = device->CreateBuffer(bufDesc, (void*)gVertices.data());

		// ======== Create Index Buffer ========
		bufDesc.BindFlags = DrawingPad::BufferBindFlags::Index;
		bufDesc.Size = static_cast<uint32_t>(sizeof(gIndices[0]) * gIndices.size());
		gIndexBuffer = device->CreateBuffer(bufDesc, (void*)gIndices.data());

		// ======== Create Uniform Buffer ========
		bufDesc.BindFlags = DrawingPad::BufferBindFlags::Uniform;
		bufDesc.Size = static_cast<uint32_t>(sizeof(UniformBufferObject) * 3);
		gUniformBuffer = device->CreateBuffer(bufDesc, nullptr);

		int width, height, channels;
		stbi_uc* pixels = stbi_load("textures/viking_room.png", &width, &height, &channels, STBI_rgb_alpha);
		DrawingPad::TextureDesc texDesc = {};
		texDesc.Type = DrawingPad::TextureType::DimTex2D;
		texDesc.Width = static_cast<uint32_t>(width);
		texDesc.Height = static_cast<uint32_t>(height);
		texDesc.MipLevels = static_cast<uint32_t>(std::floor(std::log2(std::max(width, height)))) + 1;
		texDesc.Format = DrawingPad::TextureFormat::RGBA8UnormSRGB;
		texDesc.ArraySize = 1;
		texDesc.BindFlags = DrawingPad::BindFlags::ShaderResource;
		gTexture = device->CreateTexture(texDesc, pixels);

		texDesc.Width = gGameWidth;
		texDesc.Height = gGameHeight;
		texDesc.MipLevels = 1;
		texDesc.Format = DrawingPad::TextureFormat::RGBA8Unorm;
		texDesc.BindFlags = DrawingPad::BindFlags::RenderTarget;
		gGameColorTexture = device->CreateTexture(texDesc, nullptr);

		texDesc.Format = wd->Swapchain->GetDesc().DepthFormat;
		texDesc.BindFlags = DrawingPad::BindFlags::DepthStencil;
		gGameDepthTexture = device->CreateTexture(texDesc, nullptr);

		// ======== Create Shaders ========
		DrawingPad::ShaderDesc sDesc = {};
		sDesc.EntryPoint = "main";
		sDesc.Name = "Basic Vert";
		//sDesc.Src = vertSrc;
		sDesc.Path = "shaders/ubo.vert";
		sDesc.Type = DrawingPad::ShaderType::Vertex;
		auto* vertShader = device->CreateShader(sDesc);
		sDesc.Name = "Basic Frag";
		//sDesc.Src = fragSrc;
		sDesc.Path = "shaders/ubo.frag";
		sDesc.Type = DrawingPad::ShaderType::Fragment;
		auto* fragShader = device->CreateShader(sDesc);

		std::vector<DrawingPad::LayoutElement> vertInputs = {
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

		std::vector<DrawingPad::Shader*> shaders = { vertShader, fragShader };

		DrawingPad::ShaderProgram* program = device->CreateShaderProgram(shaders);

		DrawingPad::GraphicsPipelineDesc pDesc = {
			vertInputs,
			program,
			1,
		};
		gPipeline = device->CreateGraphicsPipeline(pDesc, wd->RenderPass);
		gPipeline2 = device->CreateGraphicsPipeline(pDesc, gRenderPass);

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

		{ //BEGIN Game viewport
			ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
			ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(28.f / 255.f, 28.f / 255.f, 28.f / 255.f, 1.0f));
			ImGui::Begin("Game");
			ImVec2 viewportSize = ImGui::GetContentRegionAvail();
			// Get the scale image width and height relative to window size
			float hScale = viewportSize.y / (float)gGameHeight;
			float wScale = viewportSize.x / (float)gGameWidth;
			float width = std::min(hScale, wScale) * (float)gGameWidth;
			float height = std::min(hScale, wScale) * (float)gGameHeight;
			// Clamp to original resolution
			width = std::min(width, (float)gGameWidth);
			height = std::min(height, (float)gGameHeight);

			auto windowSize = ImGui::GetWindowSize();

			ImGui::SetCursorPos(ImVec2((viewportSize.x - width) * 0.5f + windowSize.x - viewportSize.x, (viewportSize.y - height) * 0.5f + windowSize.y - viewportSize.y));
			ImGui::Image((ImTextureID)gGameColorTexture, ImVec2(width, height));
			ImGui::End();
			ImGui::PopStyleColor();
			ImGui::PopStyleVar();
		} //END Game viewport

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
