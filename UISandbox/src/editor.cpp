#include <stdio.h>
#include <GLFW/glfw3.h>
#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <DrawingPad.h>
#include <fstream>
#include <array>
#include <vector>
#include <iomanip>
#include <chrono>
#include <iostream>
#include <glm/glm/gtx/string_cast.hpp>
#include <glm/glm/gtx/rotate_vector.hpp>
#include <imgui/imgui.h>

#include "ImGuiWindow.h"
#include "Instrumentor.h"

API Curr_API = API::Vulkan;

struct Vertex {
    glm::vec2 pos;
    glm::vec2 tex;
    uint8_t color[4];
};

struct Vtx {
    glm::vec3 pos;
    glm::vec3 color;
    glm::vec2 tex;
};

struct UniformBufferObject {
    alignas(16) glm::mat4 model;
    alignas(16) glm::mat4 view;
    alignas(16) glm::mat4 proj;
};

struct FrameRenderBuffers {
    Buffer* VertexBuffer;
    Buffer* IndexBuffer;
    Buffer* UploadBuffer;
};

struct WindowBuffers
{
    uint32_t Index;
    uint32_t Count;
    FrameRenderBuffers* FrameBuffers;
};

struct WindowData
{
    int Width;
    int Height;
    Swapchain* Swapchain;
    RenderPass* RenderPass;
    Pipeline* Pipeline;
    bool ClearEnable;
    glm::vec4 ClearValue;
    uint32_t FrameIndex;
    uint32_t ImageCount;

    WindowData()
    {
        memset(this, 0, sizeof(*this));
        ClearEnable = true;
    }
};

struct ViewportData
{
    bool WindowOwned;
    WindowData Window;
    WindowBuffers RenderBuffers;

    ViewportData() {
        WindowOwned = false;
        memset(&RenderBuffers, 0, sizeof(RenderBuffers));
    }
    ~ViewportData() {}
};

struct RenderData
{
    GraphicsDevice* Device;
    RenderPass* RenderPass;
    size_t BufferMemoryAlignment;
    Pipeline* Pipeline;
    uint32_t Subpass;

    Texture* FontImage;
    TextureView* FontView;
    Buffer* UploadBuffer;

    WindowBuffers MainWindowBuffers;

    RenderData()
    {
        memset(this, 0, sizeof(*this));
        BufferMemoryAlignment = 256;
    }
};

static GraphicsDevice* device = nullptr;
static WindowData mainWindowData;
static bool rebuildSwap = false;
static Pipeline* pipe = nullptr;
static Buffer* vb = nullptr;
static Buffer* ib = nullptr;
static Buffer* ub = nullptr;

static std::array<Texture*, 3> renderBuffers;

static void _CreateWindow(ImGuiViewport* viewport)
{
    // Different from CreateVulkanWindow
    RenderData* bd = (RenderData*)ImGui::GetIO().BackendRendererUserData;
    ViewportData* vd = new ViewportData();
    WindowData* wd = &vd->Window;
    ImGui_ImplGlfw_ViewportData* pvd = (ImGui_ImplGlfw_ViewportData*)viewport->PlatformUserData;
    Swapchain* old = wd->Swapchain;

    //GraphicsContextDesc ctxDesc = {};
    //ctxDesc.Name = "ViewportContext";
    //wd->Context = bd->Device->CreateContext(ctxDesc);
    viewport->RendererUserData = vd;


    wd->ClearEnable = (viewport->Flags & ImGuiViewportFlags_NoRendererClear) ? false : true;
    vd->WindowOwned = true;
    
    // Same as CreateVulkanWindow
    //CreaetWindowSwapChain
    wd->Width = (int)viewport->Size.x;
    wd->Height = (int)viewport->Size.y;

    SwapchainDesc swapDesc = {};
    swapDesc.Width = (uint32_t)viewport->Size.x;
    swapDesc.Height = (uint32_t)viewport->Size.y;
    swapDesc.SurfaceFormats = { TextureFormat::BGRA8Unorm, TextureFormat::RGBA8Unorm, TextureFormat::BGR8Unorm, TextureFormat::RGB8Unorm };
    swapDesc.DepthFormat = TextureFormat::None;
    wd->Swapchain = bd->Device->CreateSwapchain(swapDesc, pvd->Window);

    if (old)
        delete old;
}

static void _DestroyWindow(ImGuiViewport* viewport)
{
    RenderData* bd = (RenderData*)ImGui::GetIO().BackendRendererUserData;
    if (ViewportData* vd = (ViewportData*)viewport->RendererUserData)
    {
        if (vd->WindowOwned)
        {
            WindowData wd = vd->Window;
            delete wd.Pipeline;
            delete wd.RenderPass;
            delete wd.Swapchain;
        }
        for (uint32_t i = 0; i < vd->RenderBuffers.Count; i++)
        {
            delete vd->RenderBuffers.FrameBuffers[i].VertexBuffer;
            delete vd->RenderBuffers.FrameBuffers[i].IndexBuffer;
            delete vd->RenderBuffers.FrameBuffers[i].UploadBuffer;
        }
        delete vd->RenderBuffers.FrameBuffers;
        vd->RenderBuffers.Index = 0;
        vd->RenderBuffers.Count = 0;

        delete vd;
    }
    viewport->RendererUserData = nullptr;
}

static void _SetWindowSize(ImGuiViewport* viewport, ImVec2 size) {
    RenderData* bd = (RenderData*)ImGui::GetIO().BackendRendererUserData;
    ViewportData* vd = (ViewportData*)viewport->RendererUserData;
    ImGui_ImplGlfw_ViewportData* pvd = (ImGui_ImplGlfw_ViewportData*)viewport->PlatformUserData;
    if (vd == nullptr)
        return;
    vd->Window.ClearEnable = (viewport->Flags & ImGuiViewportFlags_NoRendererClear) ? false : true;

    WindowData* wd = &vd->Window;

    Swapchain* old = wd->Swapchain;
    wd->ImageCount = 0;
    if (wd->RenderPass)
        delete wd->RenderPass;
    if (wd->Pipeline)
        delete wd->Pipeline;

    wd->Width = (int)viewport->Size.x;
    wd->Height = (int)viewport->Size.y;

    SwapchainDesc swapDesc = {};
    swapDesc.Width = (uint32_t)viewport->Size.x;
    swapDesc.Height = (uint32_t)viewport->Size.y;
    swapDesc.SurfaceFormats = { TextureFormat::BGRA8Unorm, TextureFormat::RGBA8Unorm, TextureFormat::BGR8Unorm, TextureFormat::RGB8Unorm };
    swapDesc.DepthFormat = TextureFormat::None;
    wd->Swapchain = bd->Device->CreateSwapchain(swapDesc, pvd->Window);

    if (old)
        delete old;
}

static void _RenderWindow(ImGuiViewport* viewport, void*)
{
    RenderData* bd = (RenderData*)ImGui::GetIO().BackendRendererUserData;
    ViewportData* vd = (ViewportData*)viewport->RendererUserData;
    WindowData* wd = &vd->Window;
    //GraphicsContext* ctx = wd->Context;

    auto* rtv = wd->Swapchain->GetBackbuffer();
    //ctx->Begin(wd->Swapchain->GetImageIndex());
    //ctx->SetRenderTargets(1, &rtv, nullptr, wd->ClearEnable);
    auto clear = wd->ClearEnable ? glm::value_ptr(wd->ClearValue) : nullptr;
    //ctx->ClearColor(rtv, clear);

    // RenderData
    {
        ImDrawData* drawData = viewport->DrawData;
        int fb_width = (int)(drawData->DisplaySize.x * drawData->FramebufferScale.x);
        int fb_height = (int)(drawData->DisplaySize.y * drawData->FramebufferScale.y);
        if (fb_width <= 0 || fb_height <= 0)
            return;

        Pipeline* pipeline = wd->Pipeline;

        if (pipeline == nullptr)
            pipeline = bd->Pipeline;

        ViewportData* viewportData = (ViewportData*)viewport->DrawData->OwnerViewport->RendererUserData;
        WindowBuffers* wrb = &viewportData->RenderBuffers;
        if (wrb->FrameBuffers == nullptr)
        {
            wrb->Index = 0;
            wrb->Count = wd->ImageCount;
            wrb->FrameBuffers = (FrameRenderBuffers*)malloc(sizeof(FrameRenderBuffers) * 3);
            memset(wrb->FrameBuffers, 0, sizeof(FrameRenderBuffers) * 3);
        }

        wrb->Index = wd->Swapchain->GetImageIndex();
        auto rb = &wrb->FrameBuffers[wrb->Index];

        if (drawData->TotalVtxCount > 0)
        {
            size_t vertex_size = drawData->TotalVtxCount * sizeof(ImDrawVert);
            size_t index_size = drawData->TotalIdxCount * sizeof(ImDrawIdx);
            if (rb->VertexBuffer == nullptr || rb->VertexBuffer->GetSize() < vertex_size)
            {
                if (rb->VertexBuffer != nullptr)
                    delete rb->VertexBuffer;

                size_t sizeAligned = ((vertex_size - 1) / bd->BufferMemoryAlignment + 1) * bd->BufferMemoryAlignment;
                BufferDesc desc = {};
                desc.Size = (uint32_t)sizeAligned;
                desc.BindFlags = BufferBindFlags::Vertex;
                rb->VertexBuffer = bd->Device->CreateBuffer(desc, nullptr);
            }
            if (rb->IndexBuffer == nullptr || rb->IndexBuffer->GetSize() < index_size)
            {
                if (rb->IndexBuffer != nullptr)
                    delete rb->IndexBuffer;

                size_t sizeAligned = ((index_size - 1) / bd->BufferMemoryAlignment + 1) * bd->BufferMemoryAlignment;
                BufferDesc desc = {};
                desc.Size = (uint32_t)sizeAligned;
                desc.BindFlags = BufferBindFlags::Index;
                rb->IndexBuffer = bd->Device->CreateBuffer(desc, nullptr);
            }

            ImDrawVert* vtx_dst = nullptr;
            ImDrawIdx* idx_dst = nullptr;
            rb->VertexBuffer->MapMemory(0, rb->VertexBuffer->GetSize(), (void**)(&vtx_dst));
            rb->IndexBuffer->MapMemory(0, rb->IndexBuffer->GetSize(), (void**)(&idx_dst));
            for (int n = 0; n < drawData->CmdListsCount; n++)
            {
                const ImDrawList* cmdList = drawData->CmdLists[n];
                memcpy(vtx_dst, cmdList->VtxBuffer.Data, cmdList->VtxBuffer.Size * sizeof(ImDrawVert));
                memcpy(idx_dst, cmdList->IdxBuffer.Data, cmdList->IdxBuffer.Size * sizeof(ImDrawIdx));
                vtx_dst += cmdList->VtxBuffer.Size;
                idx_dst += cmdList->IdxBuffer.Size;
            }
            rb->VertexBuffer->FlushMemory();
            rb->IndexBuffer->FlushMemory();
        }
        // Setup Render State
        {
            //ctx->SetPipeline(pipeline);

            if (drawData->TotalVtxCount > 0)
            {
                Buffer* vertexBuffers[1] = { rb->VertexBuffer };
                uint64_t vertexOffset[1] = { 0 };
                //ctx->SetVertexBuffers(0, 1, vertexBuffers, vertexOffset);
                //ctx->SetIndexBuffer(rb->IndexBuffer, 0);
            }

            {
				/*
                Viewport viewport;
                viewport.X = 0;
                viewport.Y = 0;
                viewport.Width = (float)fb_width;
                viewport.Height = (float)fb_height;
                viewport.minDepth = 0.0f;
                viewport.maxDepth = 1.0f;
                //ctx->SetViewports(1, &viewport, fb_width, fb_height);
				*/
            }

            if (drawData->TotalVtxCount > 0) {
                float scale[2];
                scale[0] = 2.0f / drawData->DisplaySize.x;
                scale[1] = 2.0f / drawData->DisplaySize.y;
                float translate[2];
                translate[0] = -1.0f - drawData->DisplayPos.x * scale[0];
                translate[1] = -1.0f - drawData->DisplayPos.y * scale[1];

                //ctx->SetPushConstant(ShaderType::Vertex, sizeof(float) * 0, sizeof(float) * 2, scale);
                //ctx->SetPushConstant(ShaderType::Vertex, sizeof(float) * 2, sizeof(float) * 2, translate);
            }
        }

        ImVec2 clipOff = drawData->DisplayPos;
        ImVec2 clipScale = drawData->FramebufferScale;

        int globalVtxOffset = 0;
        int globalIdxOffset = 0;
        for (int n = 0; n < drawData->CmdListsCount; n++)
        {
            const ImDrawList* cmdList = drawData->CmdLists[n];
            for (int i = 0; i < cmdList->CmdBuffer.Size; i++)
            {
                const ImDrawCmd* cmd = &cmdList->CmdBuffer[i];
                ImVec2 clipMin((cmd->ClipRect.x - clipOff.x) * clipScale.x, (cmd->ClipRect.y - clipOff.y) * clipScale.y);
                ImVec2 clipMax((cmd->ClipRect.z - clipOff.x) * clipScale.x, (cmd->ClipRect.w - clipOff.y) * clipScale.y);

                if (clipMin.x < 0.0f) { clipMin.x = 0.0f; }
                if (clipMin.y < 0.0f) { clipMin.y = 0.0f; }
                if (clipMax.x > fb_width) { clipMax.x = (float)fb_width; }
                if (clipMax.y > fb_height) { clipMax.x = (float)fb_height; }
                if (clipMax.x <= clipMin.x || clipMax.y <= clipMin.y)
                    continue;

                //ctx->SetScissors(1, (uint32_t)clipMin.x, (uint32_t)clipMin.y, (uint32_t)(clipMax.x - clipMin.x), (uint32_t)(clipMax.y - clipMin.y));
                //ctx->SetShaderResource(ResourceBindingType::ImageSampler, 0, 0, (Texture*)cmd->TextureId);
                /*
				DrawIndexAttribs attribs = {};
                attribs.IndexCount = cmd->ElemCount;
                attribs.InstanceCount = 1;
                attribs.FirstIndex = cmd->IdxOffset + globalIdxOffset;
                attribs.VertexOffset = cmd->VtxOffset + globalVtxOffset;
                attribs.FirstInstance = 0;
                //ctx->DrawIndexed(attribs);
				*/
            }
            globalIdxOffset += cmdList->IdxBuffer.Size;
            globalVtxOffset += cmdList->VtxBuffer.Size;
        }

        //ctx->SetScissors(1, 0, 0, fb_width, fb_height);
    }

    //ctx->Flush();
}

static void _SwapBuffers(ImGuiViewport* viewport, void*)
{
    RenderData* bd = (RenderData*)ImGui::GetIO().BackendRendererUserData;
    ViewportData* vd = (ViewportData*)viewport->RendererUserData;
    WindowData* wd = &vd->Window;
    wd->Swapchain->Present(0);
}

static void FrameRender(WindowData* wd, ImDrawData* drawData)
{
    HZ_PROFILE_FUNCTION();
    RenderData* bd = (RenderData*)ImGui::GetIO().BackendRendererUserData;
    //auto ctx = wd->Context;
    auto rt = wd->Swapchain->GetBackbuffer();
    //ctx->Begin(wd->Swapchain->GetImageIndex());
    //ctx->SetRenderTargets(1, &rt, nullptr, false);
    //ctx->ClearColor(rt, glm::value_ptr(wd->ClearValue));
    //RenderData
    {
        HZ_PROFILE_SCOPE("RenderData");
        int fb_width = (int)(drawData->DisplaySize.x * drawData->FramebufferScale.x);
        int fb_height = (int)(drawData->DisplaySize.y * drawData->FramebufferScale.y);
        if (fb_width <= 0 || fb_height <= 0)
            return;

        ViewportData* vrd = (ViewportData*)drawData->OwnerViewport->RendererUserData;
        auto wrb = &vrd->RenderBuffers;
        if (wrb->FrameBuffers == nullptr)
        {
            wrb->Index = 0;
            wrb->Count = wd->ImageCount;
            wrb->FrameBuffers = (FrameRenderBuffers*)malloc(sizeof(FrameRenderBuffers) * wrb->Count);
            memset(wrb->FrameBuffers, 0, sizeof(FrameRenderBuffers) * wrb->Count);
        }

        wrb->Index = wd->Swapchain->GetImageIndex();
        auto rb = &wrb->FrameBuffers[wrb->Index];

        if (drawData->TotalVtxCount > 0)
        {
            HZ_PROFILE_SCOPE("CreateOrResizeBuffers");
            size_t vertex_size = drawData->TotalVtxCount * sizeof(ImDrawVert);
            size_t index_size = drawData->TotalIdxCount * sizeof(ImDrawIdx);
            if (rb->VertexBuffer == nullptr || rb->VertexBuffer->GetSize() < vertex_size)
            {
                if (rb->VertexBuffer != nullptr)
                    delete rb->VertexBuffer;

                size_t sizeAligned = ((vertex_size - 1) / bd->BufferMemoryAlignment + 1) * bd->BufferMemoryAlignment;
                BufferDesc desc = {};
                desc.Size = (uint32_t)sizeAligned;
                desc.BindFlags = BufferBindFlags::Vertex;
                rb->VertexBuffer = bd->Device->CreateBuffer(desc, nullptr);
            }
            if (rb->IndexBuffer == nullptr || rb->IndexBuffer->GetSize() < index_size)
            {
                if (rb->IndexBuffer != nullptr)
                    delete rb->IndexBuffer;

                size_t sizeAligned = ((index_size - 1) / bd->BufferMemoryAlignment + 1) * bd->BufferMemoryAlignment;
                BufferDesc desc = {};
                desc.Size = (uint32_t)sizeAligned;
                desc.BindFlags = BufferBindFlags::Index;
                rb->IndexBuffer = bd->Device->CreateBuffer(desc, nullptr);
            }

            ImDrawVert* vtx_dst = nullptr;
            ImDrawIdx* idx_dst = nullptr;
            rb->VertexBuffer->MapMemory(0, rb->VertexBuffer->GetSize(), (void**)(&vtx_dst));
            rb->IndexBuffer->MapMemory(0, rb->IndexBuffer->GetSize(), (void**)(&idx_dst));
            for (int n = 0; n < drawData->CmdListsCount; n++)
            {
                const ImDrawList* cmdList = drawData->CmdLists[n];
                memcpy(vtx_dst, cmdList->VtxBuffer.Data, cmdList->VtxBuffer.Size * sizeof(ImDrawVert));
                memcpy(idx_dst, cmdList->IdxBuffer.Data, cmdList->IdxBuffer.Size * sizeof(ImDrawIdx));
                vtx_dst += cmdList->VtxBuffer.Size;
                idx_dst += cmdList->IdxBuffer.Size;
            }
            rb->VertexBuffer->FlushMemory();
            rb->IndexBuffer->FlushMemory();
        }
        // Setup Render State
        {
            HZ_PROFILE_SCOPE("RenderState");
            //ctx->SetPipeline(bd->Pipeline);
            //ctx->SetShaderResource(ResourceBindingType::ImageSampler, 0, 0, (Texture*)bd->FontImage);
            //ctx->BindDescriptorSets();

            if (drawData->TotalVtxCount > 0)
            {
                Buffer* vertexBuffers[1] = { rb->VertexBuffer };
                uint64_t vertexOffset[1] = { 0 };
                //ctx->SetVertexBuffers(0, 1, vertexBuffers, vertexOffset);
                //ctx->SetIndexBuffer(rb->IndexBuffer, 0);
            }

            {
				/*
                Viewport viewport;
                viewport.X = 0;
                viewport.Y = 0;
                viewport.Width = (float)fb_width;
                viewport.Height = (float)fb_height;
                viewport.minDepth = 0.0f;
                viewport.maxDepth = 1.0f;
                //ctx->SetViewports(1, &viewport, fb_width, fb_height);
				*/
            }

            if (drawData->TotalVtxCount > 0) {
                float scale[2];
                scale[0] = 2.0f / drawData->DisplaySize.x;
                scale[1] = 2.0f / drawData->DisplaySize.y;
                float translate[2];
                translate[0] = -1.0f - drawData->DisplayPos.x * scale[0];
                translate[1] = -1.0f - drawData->DisplayPos.y * scale[1];
                
                //ctx->SetPushConstant(ShaderType::Vertex, sizeof(float) * 0, sizeof(float) * 2, scale);
                //ctx->SetPushConstant(ShaderType::Vertex, sizeof(float) * 2, sizeof(float) * 2, translate);
            }
        }
        {
            HZ_PROFILE_SCOPE("Render");
            ImVec2 clipOff = drawData->DisplayPos;
            ImVec2 clipScale = drawData->FramebufferScale;

            int globalVtxOffset = 0;
            int globalIdxOffset = 0;
            for (int n = 0; n < drawData->CmdListsCount; n++)
            {
                const ImDrawList* cmdList = drawData->CmdLists[n];
                for (int i = 0; i < cmdList->CmdBuffer.Size; i++)
                {
                    const ImDrawCmd* cmd = &cmdList->CmdBuffer[i];
                    ImVec2 clipMin((cmd->ClipRect.x - clipOff.x) * clipScale.x, (cmd->ClipRect.y - clipOff.y) * clipScale.y);
                    ImVec2 clipMax((cmd->ClipRect.z - clipOff.x) * clipScale.x, (cmd->ClipRect.w - clipOff.y) * clipScale.y);

                    if (clipMin.x < 0.0f) { clipMin.x = 0.0f; }
                    if (clipMin.y < 0.0f) { clipMin.y = 0.0f; }
                    if (clipMax.x > fb_width) { clipMax.x = (float)fb_width; }
                    if (clipMax.y > fb_height) { clipMax.x = (float)fb_height; }
                    if (clipMax.x <= clipMin.x || clipMax.y <= clipMin.y)
                        continue;

                    //ctx->SetScissors(1, (uint32_t)clipMin.x, (uint32_t)clipMin.y, (uint32_t)(clipMax.x - clipMin.x), (uint32_t)(clipMax.y - clipMin.y));
					/*
                    DrawIndexAttribs attribs = {};
                    attribs.IndexCount = cmd->ElemCount;
                    attribs.InstanceCount = 1;
                    attribs.FirstIndex = cmd->IdxOffset + globalIdxOffset;
                    attribs.VertexOffset = cmd->VtxOffset + globalVtxOffset;
                    attribs.FirstInstance = 0;
					*/
                    //ctx->DrawIndexed(attribs);
                }
                globalIdxOffset += cmdList->IdxBuffer.Size;
                globalVtxOffset += cmdList->VtxBuffer.Size;
            }
        }
        //ctx->SetScissors(1, 0, 0, fb_width, fb_height);
    }

    //ctx->Flush();
}

static void FramePresent(WindowData* wd)
{
    HZ_PROFILE_FUNCTION();
    wd->Swapchain->Present(0);
}

static void glfw_error_callback(int error, const char* description)
{
    fprintf(stderr, "Glfw Error %d: %s\n", error, description);
}

int main() {
    HZ_PROFILE_BEGIN_SESSION("Startup", "DrawingPadProfile-Startup.json");
    // Setup GLFW
    glfwSetErrorCallback(glfw_error_callback);
    if (!glfwInit())
        return 1;
	if (Curr_API != API::OpenGL)
		glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	GLFWwindow* window = glfwCreateWindow(1280, 720, "DrawingPad Test", NULL, NULL);

    {// Setup Vulkan
        device = GraphicsDevice::Create(window, Curr_API);
    }
	/*
    GraphicsContextDesc ctxDesc = {};
    ctxDesc.Name = "MainContext";
    ctxDesc.ContextID = 0;
	*/

    int w, h;
    glfwGetFramebufferSize(window, &w, &h);
    WindowData* wd = &mainWindowData;
    //wd->Context = device->CreateContext(ctxDesc);
    wd->Width = w;
    wd->Height = h;
    {//Setup Vulkan Window (wd, surface, w, h)
        SwapchainDesc swapDesc = {};
        swapDesc.Width = w;
        swapDesc.Height = h;
        swapDesc.SurfaceFormats = { TextureFormat::BGRA8Unorm, TextureFormat::RGBA8Unorm, TextureFormat::BGR8Unorm, TextureFormat::RGB8Unorm };
        swapDesc.DepthFormat = TextureFormat::None;

        // CreaetOrResizeWindow()
        // CreateSwapChain

        wd->Swapchain = device->CreateSwapchain(swapDesc, window);
        wd->ImageCount = 3;
    }

    {
        TextureDesc texDesc = {};
        texDesc.Type = TextureType::DimTex2D;
        texDesc.Format = TextureFormat::BGRA8Unorm;
        texDesc.Width = 400;
        texDesc.Height = 225;
        texDesc.BindFlags = BindFlags::RenderTarget;
        for (uint32_t i = 0; i < 3; i++)
        {
            renderBuffers[i] = device->CreateTexture(texDesc, nullptr);
        }
    }

    {
        // ======== Create Shaders ========
        ShaderDesc sDesc = {};
        sDesc.EntryPoint = "main";
        sDesc.Name = "Basic Vert";
        //sDesc.Src = vertSrc;
        sDesc.Path = "shaders/textured.vert";
        sDesc.Type = ShaderType::Vertex;
        auto vertShader = device->CreateShader(sDesc);
        sDesc.Name = "Basic Frag";
        //sDesc.Src = fragSrc;
        sDesc.Path = "shaders/textured.frag";
        sDesc.Type = ShaderType::Fragment;
        auto fragShader = device->CreateShader(sDesc);

        LayoutElement vertInputs[]{
            {
                0, // InputIndex Location
                0, // BufferSlot Binding
                3, // Num Components
                offsetof(Vtx, pos), // Offset
                sizeof(Vtx) // Stride
            },
            {
                1, // InputIndex Location
                0, // BufferSlot Binding
                3, // Num Components
                offsetof(Vtx, color),  // Offset
                sizeof(Vtx) // Stride
            },
            {
                2,
                0,
                2,
                offsetof(Vtx, tex),
                sizeof(Vtx)
            }
        };

        std::vector<Shader*> shaders = { vertShader, fragShader };
		/*
        GraphicsPipelineDesc pDesc = {};
        pDesc.NumViewports = 1;
        pDesc.NumColors = 1;
        pDesc.ColorFormats[0] = TextureFormat::BGRA8Unorm;
        pDesc.DepthFormat = TextureFormat::None;
        pDesc.InputLayout.NumElements = 3;
        pDesc.InputLayout.Elements = vertInputs;
        pDesc.ShaderCount = 2;
        pDesc.Shaders = shaders.data();
        pipe = device->CreateGraphicsPipeline(pDesc);
		*/
    }

    // Setup Dear ImGui
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    //io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
    io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;
    //io.ConfigViewportsNoAutoMerge = true;
    //io.ConfigViewportsNoTaskBarIcon = true;

    ImGui::StyleColorsDark();

    ImGuiStyle& style = ImGui::GetStyle();
    if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
    {
        style.WindowRounding = 0.0f;
        style.Colors[ImGuiCol_WindowBg].w = 1.0f;
    }
    
    // Setup Platform/Renderer backends
    ImGui_ImplGlfw_InitForVulkan(window, true);
    //Init Vulkan
    RenderData* bd = new RenderData();
    {
        io.BackendRendererUserData = (void*)bd; // What do we need for rendering information
        io.BackendRendererName = "DrawingPad";
        io.BackendFlags |= ImGuiBackendFlags_RendererHasVtxOffset;
        io.BackendFlags |= ImGuiBackendFlags_RendererHasViewports;
        
        bd->Device = device;

        {// CreateDeviceObjects
            // Upload Fonts
            {
                // Create Font Texture
                unsigned char* pixels;
                int width, height;
                io.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height);
                size_t upload_size = (size_t)width * (size_t)height * 4 * sizeof(char);
                {
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
                    bd->FontImage = bd->Device->CreateTexture(desc, pixels);
                }

                {
                    TextureViewDesc desc = {};
                    desc.Format = TextureFormat::RGBA8Unorm;
                    bd->FontView = bd->FontImage->CreateView(desc);
                }

                io.Fonts->TexID = (ImTextureID)bd->FontImage;
            }
            // CreatePipeline
            {
                // ======== Create Shaders ========
                std::vector<Shader*> shaders = {};
                ShaderDesc sDesc = {};
                sDesc.EntryPoint = "main";
                sDesc.Name = "UI Vert";
                sDesc.Path = "shaders/ui.vert";
                sDesc.Type = ShaderType::Vertex;
                shaders.emplace_back(bd->Device->CreateShader(sDesc));
                sDesc.Name = "UI Frag";
                sDesc.Path = "shaders/ui.frag";
                sDesc.Type = ShaderType::Fragment;
                shaders.emplace_back(bd->Device->CreateShader(sDesc));

                LayoutElement vertInputs[]{
                    {
                        0, // InputIndex Location
                        0, // BufferSlot Binding
                        2, // Num Components
                        offsetof(Vertex, pos), // Offset
                        sizeof(Vertex) // Stride
                    },
                    {
                        1, // InputIndex Location
                        0, // BufferSlot Binding
                        2, // Num Components
                        offsetof(Vertex, tex),  // Offset
                        sizeof(Vertex) // Stride
                    },
                    {
                        2,
                        0,
                        4,
                        offsetof(Vertex, color),
                        sizeof(Vertex),
                        true,
                        ElementDataType::Uint8
                    }
                };
				/*
                GraphicsPipelineDesc pipeDesc = {};
                pipeDesc.ShaderCount = (uint32_t)shaders.size();
                pipeDesc.Shaders = shaders.data();
                pipeDesc.InputLayout.NumElements = 3;
                pipeDesc.InputLayout.Elements = vertInputs;
                pipeDesc.InputLayout.NumElements = 3;
                pipeDesc.InputLayout.Elements = vertInputs;
                pipeDesc.NumViewports = 1;
                pipeDesc.NumColors = 1;
                pipeDesc.ColorFormats[0] = wd->Swapchain->GetDesc().ColorFormat;
                pipeDesc.DepthFormat = wd->Swapchain->GetDesc().DepthFormat;

                bd->Pipeline = bd->Device->CreateGraphicsPipeline(pipeDesc);
				*/
            }
        }

        ImGuiViewport* mainViewport = ImGui::GetMainViewport();
        mainViewport->RendererUserData = new ViewportData();

        if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
        {// InitPlatformInterface
            ImGuiPlatformIO& platformIo = ImGui::GetPlatformIO();
            if (ImGui::GetIO().ConfigFlags & ImGuiConfigFlags_ViewportsEnable && platformIo.Platform_CreateVkSurface == NULL)
                std::cerr << "Platform needs to setup the CreateSurface handler." << std::endl;
            platformIo.Renderer_CreateWindow = _CreateWindow;
            platformIo.Renderer_DestroyWindow = _DestroyWindow;
            platformIo.Renderer_SetWindowSize = _SetWindowSize;
            platformIo.Renderer_RenderWindow = _RenderWindow;
            platformIo.Renderer_SwapBuffers = _SwapBuffers;
        }
    }

    // Our state
    bool show_demo_window = true;
    bool show_another_window = false;
    ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);

    const std::vector<Vtx> vertices = {
        {{-0.5f, -0.5f, 0.0f}, {1.0f, 0.0f, 0.0f}, {1.0f, 0.0f}}, // BL
        {{ 0.5f, -0.5f, 0.0f}, {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f}}, //BR
        {{ 0.5f,  0.5f, 0.0f}, {0.0f, 0.0f, 1.0f}, {0.0f, 1.0f}}, // TR
        {{-0.5f,  0.5f, 0.0f}, {1.0f, 1.0f, 1.0f}, {1.0f, 1.0f}}, // TL
        {{-0.5f, -0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f}},
        {{0.5f, -0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}, {1.0f, 0.0f}},
        {{0.5f, 0.5f, -0.5f}, {0.0f, 0.0f, 1.0f}, {1.0f, 1.0f}},
        {{-0.5f, 0.5f, -0.5f}, {1.0f, 1.0f, 1.0f}, {0.0f, 1.0f}}
        //{{0.f, -1.f, 6.f}, {1.0f, 0.0f, 0.0f}, {1.0f, 0.0f}},
        //{{0.f,  5.f, 2.f}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f}},
        //{{3.f,  2.f, 1.f}, {1.0f, 0.0f, 0.0f}, {0.0f, 1.0f}},
    };

    const std::vector<uint16_t> indices = {
        0, 1, 2, 2, 3, 0
    };

    BufferDesc bufDesc = {};
    bufDesc.Usage = BufferUsageFlags::Default;
    // ======== Create Vertex Buffer ========
    bufDesc.BindFlags = BufferBindFlags::Vertex;
    bufDesc.Size = static_cast<uint32_t>(sizeof(vertices[0]) * vertices.size());
    vb = device->CreateBuffer(bufDesc, (void*)vertices.data());

    // ======== Create Index Buffer ========
    bufDesc.BindFlags = BufferBindFlags::Index;
    bufDesc.Size = static_cast<uint32_t>(sizeof(indices[0]) * indices.size());
    ib = device->CreateBuffer(bufDesc, (void*)indices.data());

    // ======== Create Uniform Buffer ========
    bufDesc.BindFlags = BufferBindFlags::Uniform;
    bufDesc.Size = static_cast<uint32_t>(sizeof(UniformBufferObject) * 3);
    bufDesc.Buffered = true;
    ub = device->CreateBuffer(bufDesc, nullptr);

    HZ_PROFILE_END_SESSION();
	/*
    DrawIndexAttribs drawAttribs = {};
    drawAttribs.IndexCount = 6;
    drawAttribs.InstanceCount = 1;
    drawAttribs.FirstIndex = 0;
    drawAttribs.FirstInstance = 0;
	*/
    HZ_PROFILE_BEGIN_SESSION("Runtime", "DrawingPadProfile-Runtime.json");
    while (!glfwWindowShouldClose(window))
    {
        HZ_PROFILE_SCOPE("RunLoop");
        glfwPollEvents();
        
        {
            HZ_PROFILE_SCOPE("FirstRender");
            //auto fb = renderBuffers[wd->Swapchain->GetImageIndex()]->GetDefaultView();
            auto fb = wd->Swapchain->GetBackbuffer();
            //auto ctx = wd->Context;
            //ctx->Begin(wd->Swapchain->GetImageIndex());
            //ctx->SetRenderTargets(1, &fb, nullptr, true);
            //ctx->ClearColor(fb, glm::value_ptr(wd->ClearValue));

            //ctx->SetPipeline(pipe);
            Buffer* vertexBuffs[] = { vb };
            uint64_t offsets[] = { 0 };
            //ctx->SetVertexBuffers(0, 1, vertexBuffs, offsets);
            //ctx->SetIndexBuffer(ib, 0);

            {
                static auto startTime = std::chrono::high_resolution_clock::now();
                auto currentTime = std::chrono::high_resolution_clock::now();
                float time = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();
                UniformBufferObject ubo = {};
                ubo.model = glm::rotate(glm::mat4(1.0f), time * glm::radians(90.f), glm::vec3(0.0f, 0.0f, 1.0f));
                //ubo.view = glm::inverse(glm::translate(glm::mat4(1.0f), glm::vec3(x, y, z)));
                //ubo.view = glm::lookAt(pos, pos+front, up);
                ubo.view = glm::lookAt(glm::vec3(2.0f, 2.0f, 2.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
                ubo.proj = glm::perspective(glm::radians(45.0f), 16.0f / 9.0f, 0.1f, 10.0f);
                ubo.proj[1][1] *= -1;
                //ubo.proj = cor * glm::ortho(-1.6f, 1.6f, -0.9f, 0.9f, -1.0f, 1.0f);
                //ctx->UploadBuffer(ub, wd->Swapchain->GetImageIndex() * sizeof(ubo), sizeof(ubo), &ubo);
            }
            //ctx->SetShaderResource(ResourceBindingType::UniformBuffer, 0, 0, ub);
            //ctx->SetShaderResource(ResourceBindingType::ImageSampler, 0, 1, texture);
            //ctx->BindDescriptorSets();
            //ctx->DrawIndexed(drawAttribs);

            //ctx->Flush();
        }
        
        // API NewFrame()
        {/*Noting to do*/}
        // GLFW NewFrame()
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

        // 3. Show another simple window.
        if (show_another_window)
        {
            ImGui::Begin("Another Window", &show_another_window);   // Pass a pointer to our bool variable (the window will have a closing button that will clear the bool when clicked)
            ImGui::Text("Hello from another window!");
            if (ImGui::Button("Close Me"))
                show_another_window = false;
            ImGui::End();
        }

        {
            //ImGui::Begin("Game", NULL, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoScrollbar);
            //ImGui::Image((ImTextureID)renderBuffers[wd->Swapchain->GetImageIndex()], ImVec2{ 400, 225 }, ImVec2(0, 1), ImVec2(1, 0));
            //ImGui::End();
        }

        ImGui::Render();
        ImDrawData* drawData = ImGui::GetDrawData();
        const bool mini = (drawData->DisplaySize.x <= 0.0f || drawData->DisplaySize.y <= 0.0f);
        wd->ClearValue = glm::vec4(clear_color.x * clear_color.w, clear_color.y * clear_color.w, clear_color.z * clear_color.w, clear_color.w);

        if (!mini)
            FrameRender(wd, drawData);

        if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
        {
            ImGui::UpdatePlatformWindows();
            ImGui::RenderPlatformWindowsDefault();
        }

        if (!mini)
            FramePresent(wd);
    }
    HZ_PROFILE_END_SESSION();

    ImGui::DestroyContext();
    //Cleanup API

    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}
