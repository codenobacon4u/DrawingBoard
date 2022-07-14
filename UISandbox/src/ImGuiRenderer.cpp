#include "ImGuiRenderer.h"

#include "ImGuiWindow.h"

#include <iostream>

// glsl_shader.vert, compiled with:
// # glslangValidator -V -x -o glsl_shader.vert.u32 glsl_shader.vert
/*
#version 450 core
layout(location = 0) in vec2 aPos;
layout(location = 1) in vec2 aUV;
layout(location = 2) in vec4 aColor;
layout(push_constant) uniform uPushConstant { vec2 uScale; vec2 uTranslate; } pc;

out gl_PerVertex { vec4 gl_Position; };
layout(location = 0) out struct { vec4 Color; vec2 UV; } Out;

void main()
{
	Out.Color = aColor;
	Out.UV = aUV;
	gl_Position = vec4(aPos * pc.uScale + pc.uTranslate, 0, 1);
}
*/
static uint32_t __glsl_shader_vert_spv[] =
{
	0x07230203,0x00010000,0x00080001,0x0000002e,0x00000000,0x00020011,0x00000001,0x0006000b,
	0x00000001,0x4c534c47,0x6474732e,0x3035342e,0x00000000,0x0003000e,0x00000000,0x00000001,
	0x000a000f,0x00000000,0x00000004,0x6e69616d,0x00000000,0x0000000b,0x0000000f,0x00000015,
	0x0000001b,0x0000001c,0x00030003,0x00000002,0x000001c2,0x00040005,0x00000004,0x6e69616d,
	0x00000000,0x00030005,0x00000009,0x00000000,0x00050006,0x00000009,0x00000000,0x6f6c6f43,
	0x00000072,0x00040006,0x00000009,0x00000001,0x00005655,0x00030005,0x0000000b,0x0074754f,
	0x00040005,0x0000000f,0x6c6f4361,0x0000726f,0x00030005,0x00000015,0x00565561,0x00060005,
	0x00000019,0x505f6c67,0x65567265,0x78657472,0x00000000,0x00060006,0x00000019,0x00000000,
	0x505f6c67,0x7469736f,0x006e6f69,0x00030005,0x0000001b,0x00000000,0x00040005,0x0000001c,
	0x736f5061,0x00000000,0x00060005,0x0000001e,0x73755075,0x6e6f4368,0x6e617473,0x00000074,
	0x00050006,0x0000001e,0x00000000,0x61635375,0x0000656c,0x00060006,0x0000001e,0x00000001,
	0x61725475,0x616c736e,0x00006574,0x00030005,0x00000020,0x00006370,0x00040047,0x0000000b,
	0x0000001e,0x00000000,0x00040047,0x0000000f,0x0000001e,0x00000002,0x00040047,0x00000015,
	0x0000001e,0x00000001,0x00050048,0x00000019,0x00000000,0x0000000b,0x00000000,0x00030047,
	0x00000019,0x00000002,0x00040047,0x0000001c,0x0000001e,0x00000000,0x00050048,0x0000001e,
	0x00000000,0x00000023,0x00000000,0x00050048,0x0000001e,0x00000001,0x00000023,0x00000008,
	0x00030047,0x0000001e,0x00000002,0x00020013,0x00000002,0x00030021,0x00000003,0x00000002,
	0x00030016,0x00000006,0x00000020,0x00040017,0x00000007,0x00000006,0x00000004,0x00040017,
	0x00000008,0x00000006,0x00000002,0x0004001e,0x00000009,0x00000007,0x00000008,0x00040020,
	0x0000000a,0x00000003,0x00000009,0x0004003b,0x0000000a,0x0000000b,0x00000003,0x00040015,
	0x0000000c,0x00000020,0x00000001,0x0004002b,0x0000000c,0x0000000d,0x00000000,0x00040020,
	0x0000000e,0x00000001,0x00000007,0x0004003b,0x0000000e,0x0000000f,0x00000001,0x00040020,
	0x00000011,0x00000003,0x00000007,0x0004002b,0x0000000c,0x00000013,0x00000001,0x00040020,
	0x00000014,0x00000001,0x00000008,0x0004003b,0x00000014,0x00000015,0x00000001,0x00040020,
	0x00000017,0x00000003,0x00000008,0x0003001e,0x00000019,0x00000007,0x00040020,0x0000001a,
	0x00000003,0x00000019,0x0004003b,0x0000001a,0x0000001b,0x00000003,0x0004003b,0x00000014,
	0x0000001c,0x00000001,0x0004001e,0x0000001e,0x00000008,0x00000008,0x00040020,0x0000001f,
	0x00000009,0x0000001e,0x0004003b,0x0000001f,0x00000020,0x00000009,0x00040020,0x00000021,
	0x00000009,0x00000008,0x0004002b,0x00000006,0x00000028,0x00000000,0x0004002b,0x00000006,
	0x00000029,0x3f800000,0x00050036,0x00000002,0x00000004,0x00000000,0x00000003,0x000200f8,
	0x00000005,0x0004003d,0x00000007,0x00000010,0x0000000f,0x00050041,0x00000011,0x00000012,
	0x0000000b,0x0000000d,0x0003003e,0x00000012,0x00000010,0x0004003d,0x00000008,0x00000016,
	0x00000015,0x00050041,0x00000017,0x00000018,0x0000000b,0x00000013,0x0003003e,0x00000018,
	0x00000016,0x0004003d,0x00000008,0x0000001d,0x0000001c,0x00050041,0x00000021,0x00000022,
	0x00000020,0x0000000d,0x0004003d,0x00000008,0x00000023,0x00000022,0x00050085,0x00000008,
	0x00000024,0x0000001d,0x00000023,0x00050041,0x00000021,0x00000025,0x00000020,0x00000013,
	0x0004003d,0x00000008,0x00000026,0x00000025,0x00050081,0x00000008,0x00000027,0x00000024,
	0x00000026,0x00050051,0x00000006,0x0000002a,0x00000027,0x00000000,0x00050051,0x00000006,
	0x0000002b,0x00000027,0x00000001,0x00070050,0x00000007,0x0000002c,0x0000002a,0x0000002b,
	0x00000028,0x00000029,0x00050041,0x00000011,0x0000002d,0x0000001b,0x0000000d,0x0003003e,
	0x0000002d,0x0000002c,0x000100fd,0x00010038
};

// glsl_shader.frag, compiled with:
// # glslangValidator -V -x -o glsl_shader.frag.u32 glsl_shader.frag
/*
#version 450 core
layout(location = 0) out vec4 fColor;
layout(set=0, binding=0) uniform sampler2D sTexture;
layout(location = 0) in struct { vec4 Color; vec2 UV; } In;
void main()
{
	fColor = In.Color * texture(sTexture, In.UV.st);
}
*/
static uint32_t __glsl_shader_frag_spv[] =
{
	0x07230203,0x00010000,0x00080001,0x0000001e,0x00000000,0x00020011,0x00000001,0x0006000b,
	0x00000001,0x4c534c47,0x6474732e,0x3035342e,0x00000000,0x0003000e,0x00000000,0x00000001,
	0x0007000f,0x00000004,0x00000004,0x6e69616d,0x00000000,0x00000009,0x0000000d,0x00030010,
	0x00000004,0x00000007,0x00030003,0x00000002,0x000001c2,0x00040005,0x00000004,0x6e69616d,
	0x00000000,0x00040005,0x00000009,0x6c6f4366,0x0000726f,0x00030005,0x0000000b,0x00000000,
	0x00050006,0x0000000b,0x00000000,0x6f6c6f43,0x00000072,0x00040006,0x0000000b,0x00000001,
	0x00005655,0x00030005,0x0000000d,0x00006e49,0x00050005,0x00000016,0x78655473,0x65727574,
	0x00000000,0x00040047,0x00000009,0x0000001e,0x00000000,0x00040047,0x0000000d,0x0000001e,
	0x00000000,0x00040047,0x00000016,0x00000022,0x00000000,0x00040047,0x00000016,0x00000021,
	0x00000000,0x00020013,0x00000002,0x00030021,0x00000003,0x00000002,0x00030016,0x00000006,
	0x00000020,0x00040017,0x00000007,0x00000006,0x00000004,0x00040020,0x00000008,0x00000003,
	0x00000007,0x0004003b,0x00000008,0x00000009,0x00000003,0x00040017,0x0000000a,0x00000006,
	0x00000002,0x0004001e,0x0000000b,0x00000007,0x0000000a,0x00040020,0x0000000c,0x00000001,
	0x0000000b,0x0004003b,0x0000000c,0x0000000d,0x00000001,0x00040015,0x0000000e,0x00000020,
	0x00000001,0x0004002b,0x0000000e,0x0000000f,0x00000000,0x00040020,0x00000010,0x00000001,
	0x00000007,0x00090019,0x00000013,0x00000006,0x00000001,0x00000000,0x00000000,0x00000000,
	0x00000001,0x00000000,0x0003001b,0x00000014,0x00000013,0x00040020,0x00000015,0x00000000,
	0x00000014,0x0004003b,0x00000015,0x00000016,0x00000000,0x0004002b,0x0000000e,0x00000018,
	0x00000001,0x00040020,0x00000019,0x00000001,0x0000000a,0x00050036,0x00000002,0x00000004,
	0x00000000,0x00000003,0x000200f8,0x00000005,0x00050041,0x00000010,0x00000011,0x0000000d,
	0x0000000f,0x0004003d,0x00000007,0x00000012,0x00000011,0x0004003d,0x00000014,0x00000017,
	0x00000016,0x00050041,0x00000019,0x0000001a,0x0000000d,0x00000018,0x0004003d,0x0000000a,
	0x0000001b,0x0000001a,0x00050057,0x00000007,0x0000001c,0x00000017,0x0000001b,0x00050085,
	0x00000007,0x0000001d,0x00000012,0x0000001c,0x0003003e,0x00000009,0x0000001d,0x000100fd,
	0x00010038
};

static ImGui_ImplDrawingPad_Data* ImGui_ImplDrawingPad_GetBackendData()
{
	return ImGui::GetCurrentContext() ? (ImGui_ImplDrawingPad_Data*)ImGui::GetIO().BackendRendererUserData : nullptr;
}

static void ImGui_ImplDrawingPad_CreateWindow(ImGuiViewport* viewport)
{
	ImGui_ImplDrawingPad_Data* bd = ImGui_ImplDrawingPad_GetBackendData();
	ImGui_ImplDrawingPad_ViewportData* vd = new ImGui_ImplDrawingPad_ViewportData();
	ImGui_ImplDrawingPad_Window* wd = &vd->Window;
	ImGui_ImplGlfw_ViewportData* pvd = (ImGui_ImplGlfw_ViewportData*)viewport->PlatformUserData;
	viewport->RendererUserData = vd;
	//CreateOrResizeWindow
	//CreateWindowSwapChain

	wd->ClearEnable = (viewport->Flags & ImGuiViewportFlags_NoRendererClear) ? false : true;
	vd->WindowOwned = false;
	wd->Width = (int)viewport->Size.x;
	wd->Height = (int)viewport->Size.y;

	SwapchainDesc swapDesc = {};
	swapDesc.Width = (uint32_t)viewport->Size.x;
	swapDesc.Height = (uint32_t)viewport->Size.y;
	swapDesc.SurfaceFormats = { TextureFormat::BGRA8Unorm, TextureFormat::RGBA8Unorm, TextureFormat::BGR8Unorm, TextureFormat::RGB8Unorm };
	swapDesc.DepthFormat = TextureFormat::None;

	wd->Swapchain = bd->Device->CreateSwapchain(swapDesc, pvd->Window);
	wd->Context = bd->Device->CreateGraphicsContext(wd->Swapchain);

	RenderPassDesc rpDesc = {};
	RenderPassAttachmentDesc attach = {};
	attach.Format = wd->Swapchain->GetDesc().ColorFormat;
	attach.Samples = SampleCount::e1Bit;
	attach.LoadOp = wd->ClearEnable ? AttachmentLoadOp::Clear : AttachmentLoadOp::DontCare;
	attach.StoreOp = AttachmentStoreOp::Store;
	attach.StencilLoadOp = AttachmentLoadOp::DontCare;
	attach.StencilStoreOp = AttachmentStoreOp::DontCare;
	attach.InitialLayout = ImageLayout::Undefined;
	attach.FinalLayout = ImageLayout::PresentSrcKHR;
	rpDesc.Attachments = { attach };
	SubpassDesc subpass = {};
	subpass.BindPoint = PipelineBindPoint::Graphics;
	subpass.ColorAttachments = { { 0, ImageLayout::ColorAttachOptimal } };
	rpDesc.Subpasses = { subpass };
	DependencyDesc dependency = {};
	dependency.SrcSubpass = ~0U;
	dependency.DstSubpass = 0;
	dependency.SrcStage = PipelineStage::ColorAttachOutput;
	dependency.DstStage = PipelineStage::ColorAttachOutput;
	dependency.SrcAccess = SubpassAccess::NA;
	dependency.DstAccess = SubpassAccess::ColorAttachWrite;
	rpDesc.SubpassDependencies = { dependency };
	wd->RenderPass = bd->Device->CreateRenderPass(rpDesc);

	// bd->RenderPass, v->MSAA, &bd->Pipeline, bd->Subpass=

	LayoutElement vertInputs[]{
		{
			0, // InputIndex Location
			0, // BufferSlot Binding
			2, // Num Components
			offsetof(ImDrawVert, pos), // Offset
			sizeof(ImDrawVert) // Stride
		},
		{
			1, // InputIndex Location
			0, // BufferSlot Binding
			2, // Num Components
			offsetof(ImDrawVert, uv),  // Offset
			sizeof(ImDrawVert) // Stride
		},
		{
			2,
			0,
			4,
			offsetof(ImDrawVert, col),
			sizeof(ImDrawVert),
			true,
			ElementDataType::Uint8
		}
	};

	GraphicsPipelineDesc pipeDesc = {};
	pipeDesc.ShaderProgram = bd->ShaderProgram;
	pipeDesc.InputLayout.NumElements = 3;
	pipeDesc.InputLayout.Elements = vertInputs;
	pipeDesc.NumViewports = 1;
	pipeDesc.DepthEnable = false;

	wd->Pipeline = bd->Device->CreateGraphicsPipeline(pipeDesc, wd->RenderPass);
}

static void ImGui_ImplDrawingPad_DestroyWindow(ImGuiViewport* viewport)
{
	ImGui_ImplDrawingPad_Data* bd = ImGui_ImplDrawingPad_GetBackendData();
	if (ImGui_ImplDrawingPad_ViewportData* vd = (ImGui_ImplDrawingPad_ViewportData*)viewport->RendererUserData)
	{
		if (vd->WindowOwned)
		{ 
			ImGui_ImplDrawingPad_Window wd = vd->Window;
			delete wd.Pipeline;
			delete wd.RenderPass;
			delete wd.Swapchain;
			delete wd.Context;
		}
		for (uint32_t i = 0; i < vd->RenderBuffers.size(); i++)
		{
			delete vd->RenderBuffers[i].VertexBuffer;
			delete vd->RenderBuffers[i].IndexBuffer;
			delete vd->RenderBuffers[i].UploadBuffer;
		}
		vd->RenderBuffers.clear();
		delete vd;
	}
	viewport->RendererUserData = nullptr;
}

static void ImGui_ImplDrawingPad_SetWindowSize(ImGuiViewport* viewport, ImVec2 size)
{
	ImGui_ImplDrawingPad_Data* bd = ImGui_ImplDrawingPad_GetBackendData();
	ImGui_ImplDrawingPad_ViewportData* vd = (ImGui_ImplDrawingPad_ViewportData*)viewport->RendererUserData;
	ImGui_ImplDrawingPad_Window* wd = &vd->Window;
	ImGui_ImplGlfw_ViewportData* pvd = (ImGui_ImplGlfw_ViewportData*)viewport->PlatformUserData;
	if (vd == nullptr)
		return;
	vd->Window.ClearEnable = (viewport->Flags & ImGuiViewportFlags_NoRendererClear) ? false : true;

	wd->Width = (int)viewport->Size.x;
	wd->Height = (int)viewport->Size.y;

	wd->Swapchain->Resize(wd->Width, wd->Height);
}

static void ImGui_ImplDrawingPad_RenderWindow(ImGuiViewport* viewport, void*)
{
	ImGui_ImplDrawingPad_ViewportData* vd = (ImGui_ImplDrawingPad_ViewportData*)viewport->RendererUserData;
	ImGui_ImplDrawingPad_Window* wd = &vd->Window;
	std::vector<ClearValue> clearValues = {};
	if (viewport->Flags & ImGuiViewportFlags_NoRendererClear)
		clearValues.push_back(wd->ClearValue);
	auto cmd = wd->Context->Begin();
	cmd->Begin();
	cmd->BeginRenderPass(wd->RenderPass, { wd->Swapchain->GetBackbuffer() }, clearValues);

	ImGui_ImplDrawingPad_RenderDrawData(viewport->DrawData, cmd, wd->Pipeline);

	cmd->EndRenderPass();
	cmd->End();

	wd->Context->Submit({ cmd });
}

static void ImGui_ImplDrawingPad_SwapBuffers(ImGuiViewport* viewport, void*)
{
	ImGui_ImplDrawingPad_ViewportData* vd = (ImGui_ImplDrawingPad_ViewportData*)viewport->RendererUserData;
	ImGui_ImplDrawingPad_Window* wd = &vd->Window;

	wd->Context->Present();
}

void ImGui_ImplDrawingPad_Init(GraphicsDevice* device, RenderPass* renderpass, uint32_t imageCount)
{
	ImGui_ImplDrawingPad_Data* bd = new ImGui_ImplDrawingPad_Data();
	ImGuiIO& io = ImGui::GetIO();
	io.BackendRendererUserData = (void*)bd;
	io.BackendRendererName = "DrawingPad";
	io.BackendFlags |= ImGuiBackendFlags_RendererHasVtxOffset;
	io.BackendFlags |= ImGuiBackendFlags_RendererHasViewports;

	bd->ImageCount = imageCount;
	bd->Device = device;
	bd->RenderPass = renderpass;
	bd->Subpass = 0;

	//CreateDeviceObjects
	{
		// bd->RenderPass, v->MSAA, &bd->Pipeline, bd->Subpass
		ShaderDesc sDesc = {};
		sDesc.EntryPoint = "main";
		sDesc.Name = "UI Vert";
		sDesc.Path = "shaders/ui.vert";
		sDesc.Type = ShaderType::Vertex;
		bd->VertexShader = bd->Device->CreateShader(sDesc);
		sDesc.Name = "UI Frag";
		sDesc.Path = "shaders/ui.frag";
		sDesc.Type = ShaderType::Fragment;
		bd->FragmentShader = bd->Device->CreateShader(sDesc);
		bd->ShaderProgram = bd->Device->CreateShaderProgram({ bd->VertexShader, bd->FragmentShader });

		LayoutElement vertInputs[]{
			{
				0, // InputIndex Location
				0, // BufferSlot Binding
				2, // Num Components
				offsetof(ImDrawVert, pos), // Offset
				sizeof(ImDrawVert) // Stride
			},
			{
				1, // InputIndex Location
				0, // BufferSlot Binding
				2, // Num Components
				offsetof(ImDrawVert, uv),  // Offset
				sizeof(ImDrawVert) // Stride
			},
			{
				2,
				0,
				4,
				offsetof(ImDrawVert, col),
				sizeof(ImDrawVert),
				true,
				ElementDataType::Uint8
			}
		};

		GraphicsPipelineDesc pipeDesc = {};
		pipeDesc.ShaderProgram = bd->ShaderProgram;
		pipeDesc.InputLayout.NumElements = 3;
		pipeDesc.InputLayout.Elements = vertInputs;
		pipeDesc.NumViewports = 1;
		pipeDesc.DepthEnable = false;

		bd->Pipeline = bd->Device->CreateGraphicsPipeline(pipeDesc, bd->RenderPass);
	}

	ImGuiViewport* main_viewport = ImGui::GetMainViewport();
	main_viewport->RendererUserData = new ImGui_ImplDrawingPad_ViewportData();

	if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
	{
		ImGuiPlatformIO& platformIO = ImGui::GetPlatformIO();
		platformIO.Renderer_CreateWindow = ImGui_ImplDrawingPad_CreateWindow;
		platformIO.Renderer_DestroyWindow = ImGui_ImplDrawingPad_DestroyWindow;
		platformIO.Renderer_SetWindowSize = ImGui_ImplDrawingPad_SetWindowSize;
		platformIO.Renderer_RenderWindow = ImGui_ImplDrawingPad_RenderWindow;
		platformIO.Renderer_SwapBuffers = ImGui_ImplDrawingPad_SwapBuffers;
	}
}

void ImGui_ImplDrawingPad_CreateFontsTexture()
{
	ImGuiIO& io = ImGui::GetIO();
	ImGui_ImplDrawingPad_Data* bd = ImGui_ImplDrawingPad_GetBackendData();

	unsigned char* pixels;
	int width, height;
	io.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height);
	size_t upload_size = uint64_t(width * height) * 4 * sizeof(char);

	TextureDesc desc = {};
	desc.Type = TextureType::DimTex2D;
	desc.Format = TextureFormat::RGBA8Unorm;
	desc.Width = width;
	desc.Height = height;
	desc.Depth = 1;
	desc.MipLevels = 1;
	desc.ArraySize = 1;
	desc.SampleCount = 1;
	desc.BindFlags = BindFlags::ShaderResource;
	bd->FontTexture = bd->Device->CreateTexture(desc, pixels);

	io.Fonts->SetTexID((ImTextureID)(intptr_t)bd->FontTexture);
}

void ImGui_ImplDrawingPad_RenderDrawData(ImDrawData* drawData, CommandBuffer* cmd, Pipeline* pipeline)
{
	int fb_width = (int)(drawData->DisplaySize.x * drawData->FramebufferScale.x);
	int fb_height = (int)(drawData->DisplaySize.y * drawData->FramebufferScale.y);
	if (fb_width <= 0 || fb_height <= 0)
		return;

	ImGui_ImplDrawingPad_Data* bd = ImGui_ImplDrawingPad_GetBackendData();
	ImGui_ImplDrawingPad_ViewportData* viewportData = (ImGui_ImplDrawingPad_ViewportData*)drawData->OwnerViewport->RendererUserData;
	assert(viewportData != nullptr);
	if (viewportData->RenderBuffers.size() < bd->ImageCount) {
		viewportData->RenderBuffers.resize(bd->ImageCount);
	}

	if (pipeline == nullptr)
		pipeline = bd->Pipeline;

	viewportData->Index = (viewportData->Index + 1) % bd->ImageCount;
	auto& rb = viewportData->RenderBuffers[viewportData->Index];
	if (drawData->TotalVtxCount > 0)
	{
		size_t vertexSize = drawData->TotalVtxCount * sizeof(ImDrawVert);
		size_t indexSize = drawData->TotalIdxCount * sizeof(ImDrawIdx);

		if (rb.VertexBuffer == nullptr || vertexSize > rb.VertexBuffer->GetSize())
		{
			if (rb.VertexBuffer != nullptr)
				delete rb.VertexBuffer;

			size_t sizeAligned = ((vertexSize - 1) / bd->BufferMemoryAlignment + 1) * bd->BufferMemoryAlignment;
			BufferDesc desc = {};
			desc.Size = (uint32_t)sizeAligned * 2;
			desc.BindFlags = BufferBindFlags::Vertex;
			rb.VertexBuffer = bd->Device->CreateBuffer(desc, nullptr);
		}
		if (rb.IndexBuffer == nullptr || indexSize > rb.IndexBuffer->GetSize())
		{
			if (rb.IndexBuffer != nullptr)
				delete rb.IndexBuffer;

			size_t sizeAligned = ((indexSize - 1) / bd->BufferMemoryAlignment + 1) * bd->BufferMemoryAlignment;
			BufferDesc desc = {};
			desc.Size = (uint32_t)sizeAligned * 2;
			desc.BindFlags = BufferBindFlags::Index;
			rb.IndexBuffer = bd->Device->CreateBuffer(desc, nullptr);
		}

		//Map Buffers
		ImDrawVert* vtxDst = (ImDrawVert*)rb.VertexBuffer->MapMemory();
		ImDrawIdx* idxDst = (ImDrawIdx*)rb.IndexBuffer->MapMemory();

		for (int i = 0; i < drawData->CmdListsCount; i++)
		{
			const ImDrawList* cmdList = drawData->CmdLists[i];
			memcpy(vtxDst, cmdList->VtxBuffer.Data, cmdList->VtxBuffer.Size * sizeof(ImDrawVert));
			memcpy(idxDst, cmdList->IdxBuffer.Data, cmdList->IdxBuffer.Size * sizeof(ImDrawIdx));
			vtxDst += cmdList->VtxBuffer.Size;
			idxDst += cmdList->IdxBuffer.Size;
		}

		// Flush Buffer
		rb.VertexBuffer->FlushMemory();
		rb.IndexBuffer->FlushMemory();
	}
	// Setup desired state
	// ImGui_ImplDrawingPad_SetRenderState(drawData, pipeline, cmd, rb, fb_width, fb_height);
	{
		cmd->BindPipeline(pipeline);

		if (drawData->TotalVtxCount > 0)
		{
			cmd->BindVertexBuffer(0, 1, { rb.VertexBuffer }, { 0 });
			cmd->BindIndexBuffer(rb.IndexBuffer, 0, sizeof(ImDrawIdx) == 2 ? 0 : 1);
		}

		cmd->SetViewports(0, 1, { { 0, 0, (float)fb_width, (float)fb_height, 0.0f, 1.0f } });

		if (drawData->TotalVtxCount > 0) {
			float scale[2]{};
			scale[0] = 2.0f / drawData->DisplaySize.x;
			scale[1] = 2.0f / drawData->DisplaySize.y;
			float translate[2]{};
			translate[0] = -1.0f - drawData->DisplayPos.x * scale[0];
			translate[1] = -1.0f - drawData->DisplayPos.y * scale[1];
			cmd->SetPushConstant(ShaderType::Vertex, sizeof(float) * 0, sizeof(float) * 2, scale);
			cmd->SetPushConstant(ShaderType::Vertex, sizeof(float) * 2, sizeof(float) * 2, translate);
		}
	}
	ImVec2 clip_off = drawData->DisplayPos;         // (0,0) unless using multi-viewports
	ImVec2 clip_scale = drawData->FramebufferScale; // (1,1) unless using retina display which are often (2,2)

	int globalVtxOffset = 0;
	int globalIdxOffset = 0;

	for (int n = 0; n < drawData->CmdListsCount; n++) {
		const ImDrawList* cmdList = drawData->CmdLists[n];
		for (int i = 0; i < cmdList->CmdBuffer.Size; i++) {
			const ImDrawCmd* pcmd = &cmdList->CmdBuffer[i];
			ImVec2 clip_min((pcmd->ClipRect.x - clip_off.x) * clip_scale.x, (pcmd->ClipRect.y - clip_off.y) * clip_scale.y);
			ImVec2 clip_max((pcmd->ClipRect.z - clip_off.x) * clip_scale.x, (pcmd->ClipRect.w - clip_off.y) * clip_scale.y);

			// Clamp to viewport as vkCmdSetScissor() won't accept values that are off bounds
			if (clip_min.x < 0.0f) { clip_min.x = 0.0f; }
			if (clip_min.y < 0.0f) { clip_min.y = 0.0f; }
			if (clip_max.x > fb_width) { clip_max.x = (float)fb_width; }
			if (clip_max.y > fb_height) { clip_max.y = (float)fb_height; }
			if (clip_max.x <= clip_min.x || clip_max.y <= clip_min.y)
				continue;

			// Apply scissor/clipping rectangle
			Rect2D scissor = {};
			scissor.offset.x = (int32_t)(clip_min.x);
			scissor.offset.y = (int32_t)(clip_min.y);
			scissor.extent.x = (uint32_t)(clip_max.x - clip_min.x);
			scissor.extent.y = (uint32_t)(clip_max.y - clip_min.y);
			cmd->SetScissors(0, 1, { scissor });

			Texture* texture = (Texture*)pcmd->TextureId;
			cmd->BindImage(texture, 0, 0);
			cmd->DrawIndexed(pcmd->ElemCount, 1, pcmd->IdxOffset + globalIdxOffset, pcmd->VtxOffset + globalVtxOffset, 0);
		}
		globalIdxOffset += cmdList->IdxBuffer.Size;
		globalVtxOffset += cmdList->VtxBuffer.Size;
	}

	cmd->SetScissors(0, 1, { { {0, 0}, { (uint32_t)fb_width, (uint32_t)fb_height } } });
}

void ImGui_ImplDrawingPad_Shutdown()
{
}

void ImGui_ImplDrawingPad_CreateOrResizeWindow()
{
}
