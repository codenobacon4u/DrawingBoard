#pragma once

#include "GraphicsDevice.h"

enum class CommandBufferType : uint32_t {
	Primary = 0,
	Secondary = 1
};

typedef struct DepthStencilValue {
	float depth;
	uint32_t stencil;
} DepthStencilValue;

typedef union ClearValue {
	float color[4];
	DepthStencilValue depthStencil;
} ClearValue;

typedef struct Viewport {
	float x, y, width, height, minDepth, maxDepth;
} Viewport;

typedef struct Rect2D {
	glm::ivec2 offset;
	glm::uvec2 extent;
} Rect2D;

class CommandBuffer
{
public:
	CommandBuffer()
	{}

	virtual ~CommandBuffer() {}

	virtual void Begin() = 0;
	virtual void BeginRenderPass(RenderPass* renderpass, std::vector<TextureView*> renderTargets, std::vector<ClearValue> clearValues) = 0;
	virtual void BindBuffer(Buffer* buffer, uint64_t offset, uint64_t range, uint32_t set, uint32_t binding, uint32_t arrayIndex = 0) = 0;
	virtual void BindImage(Texture* texture, uint32_t set, uint32_t binding, uint32_t arrayIndex = 0) = 0;
	virtual void BindIndexBuffer(Buffer* buffer, uint64_t offset, uint32_t indexType) = 0;
	virtual void BindPipeline(Pipeline* pipeline) = 0;
	virtual void BindVertexBuffer(uint32_t start, uint32_t num, std::vector<Buffer*> buffers, std::vector<uint64_t> offsets) = 0;
	virtual void Draw(uint32_t vertexCount, uint32_t instanceCount, uint32_t firstVertex, uint32_t firstInstance) = 0;
	virtual void DrawIndexed(uint32_t indexCount, uint32_t instanceCount, uint32_t firstIndex, uint32_t vertexOffset, uint32_t firstInstance) = 0;
	virtual void DrawIndexedIndirect(Buffer* buffer, uint64_t offset, uint32_t drawCount, uint32_t stride) = 0;
	virtual void DrawIndirect(Buffer* buffer, uint64_t offset, uint32_t drawCount, uint32_t stride) = 0;
	virtual void End() = 0;
	virtual void EndRenderPass() = 0;
	virtual void SetPushConstant(ShaderType type, uint32_t offset, uint32_t size, void* data) = 0;
	virtual void SetScissors(uint32_t first, uint32_t count, std::vector<Rect2D> scissors) = 0;
	virtual void SetViewports(uint32_t first, uint32_t count, std::vector<Viewport> viewports) = 0;
};
