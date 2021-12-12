#include <stdio.h>
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <DrawingPad.h>
#include <fstream>
#include <iomanip>
#include <chrono>
#include <iostream>
#include <glm/glm/gtx/string_cast.hpp>
#include <glm/glm/gtx/rotate_vector.hpp>
#include <imgui/imgui.h>

#include <imgui/backends/imgui_impl_glfw.h>

API Curr_API = API::Vulkan;

struct Vertex {
    glm::vec2 pos;
    glm::vec2 tex;
    uint8_t color[4];
};

struct UniformBufferObject {
    glm::vec2 scale;
    glm::vec2 translate;
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
    GraphicsContext* Context;
    Swapchain* Swapchain;
    RenderPass* RenderPass;
    Pipeline* Pipline;
    glm::vec4 ClearValue;
    uint32_t FrameIndex;
    uint32_t ImageCount;

    WindowData()
    {
        memset(this, 0, sizeof(*this));
    }
};

struct RenderData
{
    GraphicsDevice* Device;
    RenderPass* RenderPass;
    uint64_t BufferMemoryAlignment;
    Pipeline* Pipeline;

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

GraphicsDevice* gd;
GraphicsContext* ctx;
Shader* vertShader;
Shader* fragShader;

bool rebuildSwap = false;

static void FrameRender(WindowData* wd, RenderData* rd, ImDrawData* drawData)
{
    auto ctx = wd->Context;
    auto rt = wd->Swapchain->GetNextBackbuffer();
    ctx->Begin(wd->Swapchain->GetImageIndex());
    ctx->SetRenderTargets(1, &rt, nullptr);
    ctx->ClearColor(rt, glm::value_ptr(wd->ClearValue));

    {
        int fb_width = (int)(drawData->DisplaySize.x * drawData->FramebufferScale.x);
        int fb_height = (int)(drawData->DisplaySize.y * drawData->FramebufferScale.y);
        if (fb_width <= 0 || fb_height <= 0)
            return;

        auto wrb = &rd->MainWindowBuffers;
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
            size_t vertex_size = drawData->TotalVtxCount * sizeof(ImDrawVert);
            size_t index_size = drawData->TotalIdxCount * sizeof(ImDrawIdx);
            if (rb->VertexBuffer == nullptr || rb->VertexBuffer->GetSize() < vertex_size)
            {
                if (rb->VertexBuffer != nullptr)
                    delete rb->VertexBuffer;

                size_t sizeAligned = ((vertex_size - 1) / rd->BufferMemoryAlignment + 1) * rd->BufferMemoryAlignment;
                BufferDesc desc = {};
                desc.Size = sizeAligned;
                desc.BindFlags = BufferBindFlags::Vertex;
                rb->VertexBuffer = rd->Device->CreateBuffer(desc, nullptr);
            }
            if (rb->IndexBuffer == nullptr || rb->IndexBuffer->GetSize() < index_size)
            {
                if (rb->IndexBuffer != nullptr)
                    delete rb->IndexBuffer;

                size_t sizeAligned = ((index_size - 1) / rd->BufferMemoryAlignment + 1) * rd->BufferMemoryAlignment;
                BufferDesc desc = {};
                desc.Size = sizeAligned;
                desc.BindFlags = BufferBindFlags::Index;
                rb->IndexBuffer = rd->Device->CreateBuffer(desc, nullptr);
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
            ctx->SetPipeline(wd->Pipline);

            if (drawData->TotalVtxCount > 0)
            {
                Buffer* vertexBuffers[1] = { rb->VertexBuffer };
                uint64_t vertexOffset[1] = { 0 };
                ctx->SetVertexBuffers(0, 1, vertexBuffers, vertexOffset);
                ctx->SetIndexBuffer(rb->IndexBuffer, 0);
            }

            {
                Viewport viewport;
                viewport.X = 0;
                viewport.Y = 0;
                viewport.Width = (float)fb_width;
                viewport.Height = (float)fb_height;
                viewport.minDepth = 0.0f;
                viewport.maxDepth = 1.0f;
                ctx->SetViewports(1, &viewport, fb_width, fb_height);
            }

            if (drawData->TotalVtxCount > 0) {
                float scale[2];
                scale[0] = 2.0f / drawData->DisplaySize.x;
                scale[1] = 2.0f / drawData->DisplaySize.y;
                float translate[2];
                translate[0] = -1.0f - drawData->DisplayPos.x * scale[0];
                translate[1] = -1.0f - drawData->DisplayPos.y * scale[1];
                UniformBufferObject ubo = {};
                ubo.scale = glm::vec2(scale[0], scale[1]);
                ubo.translate = glm::vec2(translate[0], translate[1]);
                
                ctx->SetPushConstant(ShaderType::Vertex, sizeof(float) * 0, sizeof(float) * 2, scale);
                ctx->SetPushConstant(ShaderType::Vertex, sizeof(float) * 2, sizeof(float) * 2, translate);

                ctx->SetShaderResource(ResourceBindingType::ImageSampler, 0, 0, rd->FontImage);
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

                ctx->SetScissors(1, (uint32_t)clipMin.x, (uint32_t)clipMin.y, (uint32_t)(clipMax.x - clipMin.x), (uint32_t)(clipMax.y - clipMin.y));

                DrawIndexAttribs attribs = {};
                attribs.IndexCount = cmd->ElemCount;
                attribs.InstanceCount = 1;
                attribs.FirstIndex = cmd->IdxOffset + globalIdxOffset;
                attribs.VertexOffset = cmd->VtxOffset + globalVtxOffset;
                attribs.FirstInstance = 0;
                ctx->DrawIndexed(attribs);
            }
            globalIdxOffset += cmdList->IdxBuffer.Size;
            globalVtxOffset += cmdList->VtxBuffer.Size;
        }
        
        ctx->SetScissors(1, 0, 0, fb_width, fb_height);
    }

    ctx->Flush();
}

static void FramePresent(WindowData* wd)
{
    wd->Swapchain->Present(0);
}

int main() {
    if (!glfwInit())
        return 1;
	if (Curr_API != API::OpenGL)
		glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	GLFWwindow* window = glfwCreateWindow(1280, 720, "DrawingPad Test", NULL, NULL);
	if (window == NULL) {
		printf("Window is NULL!\n");
		std::abort();
	}

    gd = GraphicsDevice::Create(window, Curr_API);

    GraphicsContextDesc ctxDesc = {};
    ctx = gd->CreateContext(ctxDesc);

    int w, h;
    glfwGetFramebufferSize(window, &w, &h);
    SwapchainDesc swapDesc = {};
    swapDesc.Width = w;
    swapDesc.Height = h;
    swapDesc.SurfaceFormats = { TextureFormat::BGRA8Unorm, TextureFormat::RGBA8Unorm, TextureFormat::BGR8Unorm, TextureFormat::RGB8Unorm };
    swapDesc.DepthFormat = TextureFormat::None;

    WindowData wData = {};
    wData.Context = ctx;
    wData.Swapchain = gd->CreateSwapchain(swapDesc, ctx, window);

    // ======== Create Shaders ========
    ShaderDesc sDesc = {};
    sDesc.EntryPoint = "main";
    sDesc.Name = "Basic Vert";
    //sDesc.Src = vertSrc;
    sDesc.Path = "shaders/ui.vert";
    sDesc.Type = ShaderType::Vertex;
    vertShader = gd->CreateShader(sDesc);
    sDesc.Name = "Basic Frag";
    //sDesc.Src = fragSrc;
    sDesc.Path = "shaders/ui.frag";
    sDesc.Type = ShaderType::Fragment;
    fragShader = gd->CreateShader(sDesc);

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

    GraphicsPipelineDesc pipeDesc = {};
    pipeDesc.NumViewports = 1;
    pipeDesc.NumColors = 1;
    pipeDesc.ColorFormats[0] = wData.Swapchain->GetDesc().ColorFormat;
    pipeDesc.DepthFormat = wData.Swapchain->GetDesc().DepthFormat;
    pipeDesc.InputLayout.NumElements = 3;
    pipeDesc.InputLayout.Elements = vertInputs;
    pipeDesc.ShaderCount = 2;
    pipeDesc.Shaders[0] = vertShader;
    pipeDesc.Shaders[1] = fragShader;
    wData.Pipline = gd->CreateGraphicsPipeline(pipeDesc);

    wData.ImageCount = 3;

    // Setup Dear ImGui
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    ImGui::StyleColorsDark();
    
    // Setup Platform/Renderer backents
    ImGui_ImplGlfw_InitForVulkan(window, true);
    //io.BackendFlags |= ImGuiBackendFlags_RendererHasVtxOffset;

    RenderData data = {};
    data.Device = gd;

    // Upload Fonts
    {
        // Get Command Buffer
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
            data.FontImage = gd->CreateTexture(desc, pixels);
        }

        {
            TextureViewDesc desc = {};
            desc.Format = TextureFormat::RGBA8Unorm;
            data.FontView = data.FontImage->CreateView(desc);
        }

        io.Fonts->TexID = (ImTextureID)data.FontImage;
    }
    // Our state
    bool show_demo_window = true;
    bool show_another_window = false;
    ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);
    while (!glfwWindowShouldClose(window))
    {
        glfwPollEvents();

        if (rebuildSwap)
        {
            //resize
            int width, height;
            glfwGetFramebufferSize(window, &width, &height);
            if (width > 0 && height > 0)
            {

                rebuildSwap = false;
            }
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

        ImGui::Render();
        ImDrawData* drawData = ImGui::GetDrawData();
        const bool mini = (drawData->DisplaySize.x <= 0.0f || drawData->DisplaySize.y <= 0.0f);

        if (!mini)
        {
            wData.ClearValue = glm::vec4(clear_color.x * clear_color.w, clear_color.y * clear_color.w, clear_color.z * clear_color.w, clear_color.w);
            FrameRender(&wData, &data, drawData);
            FramePresent(&wData);
        }
    }

    ImGui::DestroyContext();
    //Cleanup API

    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}