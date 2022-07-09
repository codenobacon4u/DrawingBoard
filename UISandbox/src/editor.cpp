#include <stdio.h>
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <DrawingPad.h>
#include <thread>
#include <fstream>
#include <chrono>
#include <iostream>
#include <glm/glm/gtx/string_cast.hpp>
#include <glm/glm/gtx/rotate_vector.hpp>

#pragma warning(push, 0)
#pragma warning( disable: 26451 )
#pragma warning( disable: 6262 )
#pragma warning( disable: 26498 )
#pragma warning( disable: 26819 )
#pragma warning( disable: 26495 )
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#define TINYOBJLOADER_IMPLEMENTATION
#include <tiny_obj_loader.h>
#pragma warning(pop)

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

static GraphicsDevice* device;

static Swapchain* gSwapchain;
static Buffer* gVertexBuffer;
static Buffer* gIndexBuffer;
static Buffer* gGameUniformBuffer;
static Buffer* gSceneUniformBuffer;
static Pipeline* gPipeline;
static RenderPass* gRenderPass;

static Texture* gTexture;

static Texture* gGameColorTexture;
static Texture* gGameDepthTexture;
static uint32_t gGameWidth = 1280;
static uint32_t gGameHeight = 720;


static Texture* gSceneColorTexture;
static Texture* gSceneDepthTexture;
static uint32_t gSceneWidth = 1280;
static uint32_t gSceneHeight = 720;

static std::vector<Vertex> gVertices;
static std::vector<uint32_t> gIndices;
static Texture* oldColor = nullptr;
static Texture* oldDepth = nullptr;

static bool gameViewable = false;
static bool sceneViewable = false;

static bool vsync = false;

static void glfw_error_callback(int error, const char* desc) {
	std::cerr << "GLFW Error: [" << error << "] " << desc;
}

void RenderGame(CommandBuffer* cmd, TextureView* rtv) {
	cmd->SetViewports(0, 1, { { 0, 0, (float)gGameWidth, (float)gGameHeight, 0.0f, 1.0f } });
	cmd->SetScissors(0, 1, { { { 0, 0 }, { gGameWidth, gGameHeight } } });
	cmd->BindPipeline(gPipeline);
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

void RenderScene(CommandBuffer* cmd, TextureView* rtv) {
	cmd->SetViewports(0, 1, { { 0, 0, (float)gSceneWidth, (float)gSceneHeight, 0.0f, 1.0f } });
	cmd->SetScissors(0, 1, { { { 0, 0 }, { gSceneWidth, gSceneHeight } } });
	cmd->BindPipeline(gPipeline);
	cmd->BindVertexBuffer(0, 1, { gVertexBuffer }, { 0 });
	cmd->BindIndexBuffer(gIndexBuffer, 0, 1);

	{
		static auto startTime = std::chrono::high_resolution_clock::now();
		auto currentTime = std::chrono::high_resolution_clock::now();
		float time = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();
		UniformBufferObject ubo = {};
		ubo.model = glm::rotate(glm::mat4(1.0f), time * glm::radians(90.f), glm::vec3(0.0f, 0.0f, 1.0f));
		ubo.view = glm::lookAt(glm::vec3(-2.0f, -2.0f, 2.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
		ubo.proj = glm::perspective(glm::radians(45.0f), (float)gSceneWidth / (float)gSceneHeight, 0.1f, 10.0f);
		ubo.proj[1][1] *= -1;

		gSceneUniformBuffer->Update(gSwapchain->GetImageIndex() * sizeof(ubo), sizeof(ubo), reinterpret_cast<uint8_t*>(&ubo));

		cmd->BindBuffer(gSceneUniformBuffer, gSwapchain->GetImageIndex() * sizeof(ubo), sizeof(ubo), 0, 0);
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

	// Begin CommandBuffer
	cmd->Begin();
	
	if (gameViewable) {
		// Record Game pass
		cmd->BeginRenderPass(gRenderPass, { gGameColorTexture->GetDefaultView(), gGameDepthTexture->GetDefaultView() }, { {0.0f, 0.0f, 0.0f, 1.0f}, {1.0f, 0} });
		RenderGame(cmd, gGameColorTexture->GetDefaultView());
		cmd->EndRenderPass();
	}

	if (sceneViewable) {
		// Resize Scene viewport
		if (gSceneColorTexture->GetDesc().Width != gSceneWidth || gSceneColorTexture->GetDesc().Height != gSceneHeight) {
			if (oldColor) {
				device->WaitForIdle();
				delete oldColor;
				delete oldDepth;
			}

			TextureDesc modColor = gSceneColorTexture->GetDesc();
			TextureDesc modDepth = gSceneDepthTexture->GetDesc();
			modColor.Width = gSceneWidth;
			modColor.Height = gSceneHeight;
			modDepth.Width = gSceneWidth;
			modDepth.Height = gSceneHeight;
			oldColor = gSceneColorTexture;
			oldDepth = gSceneDepthTexture;

			gSceneColorTexture = device->CreateTexture(modColor, nullptr);
			gSceneDepthTexture = device->CreateTexture(modDepth, nullptr);
		}
		// Record Scene pass
		cmd->BeginRenderPass(gRenderPass, { gSceneColorTexture->GetDefaultView(), gSceneDepthTexture->GetDefaultView() }, { {0.0f, 0.0f, 0.0f, 1.0f}, {1.0f, 0} });
		RenderScene(cmd, gSceneColorTexture->GetDefaultView());
		cmd->EndRenderPass();
	}
	// Record ImGui primitives
	cmd->BeginRenderPass(windowData->RenderPass, { rtv, dsv }, { windowData->ClearValue, { 1.0f, 0 } });
	ImGui_ImplDrawingPad_RenderDrawData(drawData, cmd);
	cmd->EndRenderPass();

	// End & Submit CommandBuffer
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
	device = GraphicsDevice::Create(window, Curr_API);

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


		glfwSetWindowUserPointer(window, wd->Swapchain);
		glfwSetFramebufferSizeCallback(window, [](GLFWwindow* win, int width, int height) {
			Swapchain* swap = (Swapchain*)glfwGetWindowUserPointer(win);
			swap->SetResized(width, height);
		});

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

			{
				RenderPassDesc rpDesc = {};
				std::vector<RenderPassAttachmentDesc> attachments = {
					{
						TextureFormat::RGBA8Unorm,
						1,
						AttachmentLoadOp::Clear,
						AttachmentStoreOp::Store,
						AttachmentLoadOp::DontCare,
						AttachmentStoreOp::DontCare,
						ImageLayout::Undefined,
						ImageLayout::ShaderReadOnlyOptimal
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

				std::vector<DependencyDesc> dependencies = {
					{
						~0U,
						0,
						PipelineStage::FragmentShader,
						PipelineStage::ColorAttachOutput,
						SubpassAccess::ShaderRead,
						SubpassAccess::ColorAttachWrite
					},
					{
						0,
						~0U,
						PipelineStage::ColorAttachOutput,
						PipelineStage::FragmentShader,
						SubpassAccess::ColorAttachWrite,
						SubpassAccess::ShaderRead
					}
				};

				rpDesc.Attachments = attachments;
				rpDesc.Subpasses = { subpass };
				rpDesc.SubpassDependencies = dependencies;
				gRenderPass = device->CreateRenderPass(rpDesc);
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
		gGameUniformBuffer = device->CreateBuffer(bufDesc, nullptr);
		gSceneUniformBuffer = device->CreateBuffer(bufDesc, nullptr);

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

		texDesc.Width = gGameWidth;
		texDesc.Height = gGameHeight;
		texDesc.MipLevels = 1;
		texDesc.Format = TextureFormat::RGBA8Unorm;
		texDesc.BindFlags = BindFlags::RenderTarget;
		gGameColorTexture = device->CreateTexture(texDesc, nullptr);
		gSceneColorTexture = device->CreateTexture(texDesc, nullptr);

		texDesc.Format = wd->Swapchain->GetDesc().DepthFormat;
		texDesc.BindFlags = BindFlags::DepthStencil;
		gGameDepthTexture = device->CreateTexture(texDesc, nullptr);
		gSceneDepthTexture = device->CreateTexture(texDesc, nullptr);


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

		auto* program = device->CreateShaderProgram(vertShader, fragShader);

		GraphicsPipelineDesc pDesc = {};
		pDesc.NumViewports = 1;
		pDesc.NumColors = 1;
		pDesc.ColorFormats[0] = wd->Swapchain->GetDesc().ColorFormat;
		pDesc.DepthFormat = wd->Swapchain->GetDesc().DepthFormat;
		pDesc.InputLayout.NumElements = 3;
		pDesc.InputLayout.Elements = vertInputs;
		pDesc.Program = program;
		pDesc.Face = FrontFace::CounterClockwise;
		//pDesc.MSAASamples = 4;
		gPipeline = device->CreateGraphicsPipeline(pDesc, gRenderPass);

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

		// Start the Dear ImGui frame
		// ImplDrawingPad_NewFrame();
		ImGui_ImplGlfw_NewFrame();
		ImGui::NewFrame();

		// Rendering
		
		static bool dockspaceOpen = true;
		static bool opt_fullscreen_p = true;
		static ImGuiDockNodeFlags dockspace_flags = ImGuiDockNodeFlags_None;
		bool opt_fullscreen = opt_fullscreen_p;

		ImGuiWindowFlags window_flags = ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoDocking;
		if (opt_fullscreen) {
			ImGuiViewport* view = ImGui::GetMainViewport();
			ImGui::SetNextWindowPos(view->Pos);
			ImGui::SetNextWindowSize(view->Size);
			ImGui::SetNextWindowViewport(view->ID);
			ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
			ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
			window_flags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize;
			window_flags |= ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoNavFocus;
		}

		if (dockspace_flags & ImGuiDockNodeFlags_PassthruCentralNode)
			window_flags |= ImGuiWindowFlags_NoBackground;

		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
		ImGui::Begin("DockSpace", &dockspaceOpen, window_flags);
		ImGui::PopStyleVar();

		if (opt_fullscreen)
			ImGui::PopStyleVar(2);

		auto& io = ImGui::GetIO();
		auto& style = ImGui::GetStyle();
		float minWinX = style.WindowMinSize.x;
		style.WindowMinSize.x = 370.0f;
		if (io.ConfigFlags & ImGuiConfigFlags_DockingEnable) {
			ImGuiID id = ImGui::GetID("DockSpace");
			ImGui::DockSpace(id, ImVec2(0.0f, 0.0f), dockspace_flags);
		}

		style.WindowMinSize.x = minWinX;

		if (ImGui::BeginMenuBar()) {
			if (ImGui::BeginMenu("File")) {
				ImGui::MenuItem("New", "Ctrl+N");
				ImGui::MenuItem("Open...", "Ctrl+O");
				ImGui::MenuItem("Save", "Ctrl+S");
				ImGui::MenuItem("Save As...", "Ctrl+Shift+S");
				if(ImGui::MenuItem("Exit"))
					glfwSetWindowShouldClose(window, 1);
				ImGui::EndMenu();
			}
			ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
			ImGui::EndMenuBar();
		}
		
		{ //BEGIN Game viewport
			ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
			ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(28.f / 255.f, 28.f / 255.f, 28.f / 255.f, 1.0f));
			if (gameViewable = ImGui::Begin("Game")) {
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
			}
			ImGui::End();
			ImGui::PopStyleColor();
			ImGui::PopStyleVar();
		} //END Game viewport

		{ //BEGIN Scene viewport
			ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
			ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(28.f / 255.f, 28.f / 255.f, 28.f / 255.f, 1.0f));
			if (sceneViewable = ImGui::Begin("Scene")) {
				ImVec2 viewportSize = ImGui::GetContentRegionAvail();

				gSceneWidth = (uint32_t)viewportSize.x;
				gSceneHeight = (uint32_t)viewportSize.y;

				ImGui::Image((ImTextureID)gSceneColorTexture, viewportSize);
			}
			ImGui::End();
			ImGui::PopStyleColor();
			ImGui::PopStyleVar();
		} //END Game viewport

		ImGui::Begin("Debug");
		ImGui::Text("Game Resolution: %d x %d", gGameWidth, gGameHeight);
		ImGui::Text("Game Visible: %s", gameViewable ? "True" : "False");
		ImGui::Text("Scene Resolution: %d x %d", gSceneWidth, gSceneHeight);
		ImGui::Text("Scene Visible: %s", sceneViewable ? "True" : "False");
		ImGui::End();

		ImGui::End();
		
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
