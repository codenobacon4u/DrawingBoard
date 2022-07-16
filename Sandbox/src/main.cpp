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

API Curr_API = API::Vulkan;

static std::string vertSrc = R"(
#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec2 inPos;
layout(location = 1) in vec3 inColor;
 
layout(location = 0) out vec3 fragColor;


void main() {
	gl_Position = vec4(inPos, 0.0, 1.0);
	fragColor = inColor;
}
)";

static std::string fragSrc = R"(
#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec3 fragColor;

layout(location = 0) out vec4 outColor;

void main() {
	outColor = vec4(fragColor, 1.0);
}
)";

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

void UpdateUniformBuffer(uint32_t index, uint32_t width, uint32_t height)
{
	static auto startTime = std::chrono::high_resolution_clock::now();
	auto currTime = std::chrono::high_resolution_clock::now();
	float time = std::chrono::duration<float, std::chrono::seconds::period>(currTime - startTime).count();

	UniformBufferObject ubo = {};
	ubo.model = glm::rotate(glm::mat4(1.0f), time * glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
	ubo.view = glm::lookAt(glm::vec3(2.0f, 2.0f, 2.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
	ubo.proj = glm::perspective(glm::radians(45.0f), width / (float) height, 0.1f, 10.0f);
	ubo.proj[1][1] += -1;
}

void printMat(glm::mat4 mat)
{
	int i, j;
	for (j = 0; j < 4; j++) {
		for (i = 0; i < 4; i++) {
			printf("%f ", mat[i][j]);
		}
		printf("\n");
	}
}

glm::vec3 pos = glm::vec3(0.0f, 0.0f, 0.0f);
glm::vec3 front = glm::vec3(0.0f, 0.0f, 1.0f);
glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f);

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
	if (key == GLFW_KEY_W && action == GLFW_PRESS)
		pos += 0.05f * front;
	if (key == GLFW_KEY_A && action == GLFW_PRESS)
		pos -= 0.05f * glm::normalize(glm::cross(front, up));
	if (key == GLFW_KEY_S && action == GLFW_PRESS)
		pos -= 0.05f * front;
	if (key == GLFW_KEY_D && action == GLFW_PRESS)
		pos += 0.05f * glm::normalize(glm::cross(front, up));
}

int main() 
{
	remove("validation_layers.log");
	GraphicsDevice* gd;
	GraphicsContext* ctx;
	Swapchain* swap;
	Buffer* vb;
	Buffer* ib;
	Buffer* ub;
	RenderPass* rp;
	Pipeline* pipeline;
	Shader* vertShader;
	Shader* fragShader;
	if (!glfwInit())
		printf("Failed");
	if (Curr_API != API::OpenGL)
		glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	GLFWwindow* window = glfwCreateWindow(1920, 1080, "DrawingPad Test", NULL, NULL);
	if (window == NULL) {
		printf("Window is NULL!\n");
		std::abort();
	}
	glfwSetKeyCallback(window, key_callback);

	gd = GraphicsDevice::Create(window, Curr_API);

	SwapchainDesc swapDesc;
	swap = gd->CreateSwapchain(swapDesc, window);
	ctx = gd->CreateGraphicsContext(swap);
	
	RenderPassDesc rpDesc = {};
	std::vector<RenderPassAttachmentDesc> attachments = {
		{
			swap->GetDesc().ColorFormat,
			1,
			AttachmentLoadOp::Clear,
			AttachmentStoreOp::Store,
			AttachmentLoadOp::Discard,
			AttachmentStoreOp::Discard,
			ImageLayout::Undefined,
			ImageLayout::PresentSrcKHR
		},
		{
			swap->GetDesc().DepthFormat,
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
	subpass.ColorAttachments = { { 0, ImageLayout::ColorAttachOptimal } };
	subpass.DepthStencilAttachment = &depthAttach;
	subpass.PreserveAttachments = {};

	DependencyDesc dependency = {};
	dependency.SrcSubpass = ~0U;
	dependency.DstSubpass = 0;
	dependency.SrcStage = (PipelineStage)(PipelineStage::ColorAttachOutput | PipelineStage::EarlyFragTests);
	dependency.DstStage = (PipelineStage)(PipelineStage::ColorAttachOutput | PipelineStage::EarlyFragTests);
	dependency.SrcAccess = SubpassAccess::NA;
	dependency.DstAccess = (SubpassAccess)(SubpassAccess::ColorAttachWrite | SubpassAccess::DepthStencilAttachWrite);

	rpDesc.Attachments = attachments;
	rpDesc.Subpasses = { subpass };
	rpDesc.SubpassDependencies = { dependency };
	rp = gd->CreateRenderPass(rpDesc);
	
	std::vector<Vertex> vertices; /*= {
		{{-0.5f, -0.5f, 0.0f}, {1.0f, 0.0f, 0.0f}, {1.0f, 0.0f}}, // BL
		{{ 0.5f, -0.5f, 0.0f}, {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f}}, //BR
		{{ 0.5f,  0.5f, 0.0f}, {0.0f, 0.0f, 1.0f}, {0.0f, 1.0f}}, // TR
		{{-0.5f,  0.5f, 0.0f}, {1.0f, 1.0f, 1.0f}, {1.0f, 1.0f}}, // TL

		{{-0.5f, -0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}, {1.0f, 0.0f}},
		{{ 0.5f, -0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f}},
		{{ 0.5f,  0.5f, -0.5f}, {0.0f, 0.0f, 1.0f}, {0.0f, 1.0f}},
		{{-0.5f,  0.5f, -0.5f}, {1.0f, 1.0f, 1.0f}, {1.0f, 1.0f}},\
	};*/

	std::vector<uint32_t> indices; /*= {
		0, 1, 2, 2, 3, 0,
		4, 5, 6, 6, 7, 4
	};*/
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
					uniqueVertices[hash] = static_cast<uint32_t>(vertices.size());
					vertices.push_back(vertex);
				}

				indices.push_back(uniqueVertices[hash]);
			}
		}
	}

	BufferDesc bufDesc = {};
	bufDesc.Usage = BufferUsageFlags::Default;
	// ======== Create Vertex Buffer ========
	bufDesc.BindFlags = BufferBindFlags::Vertex;
	bufDesc.Size = static_cast<uint32_t>(sizeof(vertices[0]) * vertices.size());
	vb = gd->CreateBuffer(bufDesc, (void*)vertices.data());

	// ======== Create Index Buffer ========
	bufDesc.BindFlags = BufferBindFlags::Index;
	bufDesc.Size = static_cast<uint32_t>(sizeof(indices[0]) * indices.size());
	ib = gd->CreateBuffer(bufDesc, (void*)indices.data());

	// ======== Create Uniform Buffer ========
	bufDesc.BindFlags = BufferBindFlags::Uniform;
	bufDesc.Size = static_cast<uint32_t>(sizeof(UniformBufferObject) * 3);
	ub = gd->CreateBuffer(bufDesc, nullptr);

	// ======== Create Shaders ========
	ShaderDesc sDesc = {};
	sDesc.EntryPoint = "main";
	sDesc.Name = "Basic Vert";
	//sDesc.Src = vertSrc;
	sDesc.Path = "shaders/ubo.vert";
	sDesc.Type = ShaderType::Vertex;
	vertShader = gd->CreateShader(sDesc);
	sDesc.Name = "Basic Frag";
	//sDesc.Src = fragSrc;
	sDesc.Path = "shaders/ubo.frag";
	sDesc.Type = ShaderType::Fragment;
	fragShader = gd->CreateShader(sDesc);

	std::vector<LayoutElement> vertInputs = {
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

	ShaderProgram* program = gd->CreateShaderProgram(shaders);

	GraphicsPipelineDesc pDesc = {
		vertInputs,
		program,
		1,
		true,
	};

	pipeline = gd->CreateGraphicsPipeline(pDesc, rp);

	//Texture* texture = gd->GetTextureManager()->GetTexture("textures/texture.jpg", TextureFormat::RGBA8UnormSRGB);
	//const unsigned char* data = (const unsigned char*)malloc(256 * 256 * 4);
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
	Texture* texture = gd->CreateTexture(texDesc, pixels);

	auto glfwVersion = glfwGetVersionString();
	if (Curr_API == API::OpenGL) {
		glfwMakeContextCurrent(window);
		gladLoadGLLoader((GLADloadproc)glfwGetProcAddress);
		auto glVersion = glGetString(GL_VERSION);
		auto glRenderer = glGetString(GL_RENDERER);
		auto glVendor = glGetString(GL_VENDOR);
		printf("GLFW Version: %s\nGL Version: %s\nGL Renderer: %s\nGL Vendor: %s\n", glfwVersion, glVersion, glRenderer, glVendor);
	}

	/*DrawIndexAttribs drawAttribs = {};
	drawAttribs.IndexCount = static_cast<uint32_t>(indices.size());
	drawAttribs.InstanceCount = 1;
	drawAttribs.FirstIndex = 0;
	drawAttribs.FirstInstance = 0;*/

	while (!glfwWindowShouldClose(window))
	{
		//====UPDATE====
		glfwPollEvents();

		//====RENDER====
		auto cmd = ctx->Begin();
		TextureView* rtv = swap->GetBackbuffer();
		TextureView* dsv = swap->GetDepthBufferView();
		float color[4] = { 0.f, 0.f, 0.f, 1.0f };
		Viewport vp = { 0, 0, static_cast<float>(rtv->GetTexture()->GetDesc().Width), static_cast<float>(rtv->GetTexture()->GetDesc().Height), 0.0f, 1.0f };
		Rect2D sc = { { 0, 0 }, { rtv->GetTexture()->GetDesc().Width, rtv->GetTexture()->GetDesc().Height } };
		cmd->Begin();
		cmd->BeginRenderPass(rp, { rtv, dsv }, { { 0.0f, 0.0f, 0.0f, 1.0f }, { 1.0f, 0 } });
		cmd->SetViewports(0, 1, { vp });
		cmd->SetScissors(0, 1, { sc });
		cmd->BindPipeline(pipeline);
		std::vector<Buffer*> vertexBuffs = { vb };
		std::vector<uint64_t> offsets = { 0 };
		cmd->BindVertexBuffer(0, 1, vertexBuffs, offsets);
		cmd->BindIndexBuffer(ib, 0, 1);
		
		{
			static auto startTime = std::chrono::high_resolution_clock::now();
			auto currentTime = std::chrono::high_resolution_clock::now();
			float time = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();
			UniformBufferObject ubo = {};
			ubo.model = glm::rotate(glm::mat4(1.0f), time * glm::radians(90.f), glm::vec3(0.0f, 0.0f, 1.0f));
			ubo.view = glm::lookAt(glm::vec3(2.0f, 2.0f, 2.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
			ubo.proj = glm::perspective(glm::radians(45.0f), 16.0f / 9.0f, 0.1f, 10.0f);
			ubo.proj[1][1] *= -1;
			
			ub->Update(swap->GetImageIndex() * sizeof(ubo), sizeof(ubo), &ubo);

			cmd->BindBuffer(ub, swap->GetImageIndex() * sizeof(ubo), sizeof(ubo), 0, 0);
		}

		cmd->BindImage(texture, 0, 1);
		cmd->DrawIndexed(static_cast<uint32_t>(indices.size()), 1, 0, 0, 0);
		cmd->EndRenderPass();
		cmd->End();
		ctx->Submit({ cmd });

		//====PRESENT====
		ctx->Present();
	}
	gd->WaitForIdle();

	delete vertShader;
	delete fragShader;
	delete program;
	delete pipeline;
	delete rp;
	delete swap;
	delete ub;
	delete texture;
	delete ib;
	delete vb;
	delete ctx;
	delete gd;

	glfwDestroyWindow(window);
	glfwTerminate();

	_CrtDumpMemoryLeaks();
}
