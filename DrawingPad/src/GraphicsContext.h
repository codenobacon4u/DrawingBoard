#pragma once

#include "CommandBuffer.h"
#include "GraphicsDevice.h"

class GraphicsContext
{
public:
	GraphicsContext(GraphicsDevice* device, Swapchain* swap) {}

	virtual ~GraphicsContext() {}

	virtual CommandBuffer* Begin() = 0;
	virtual void Submit(CommandBuffer& commandBuffer) = 0;
	virtual void Submit(const std::vector<CommandBuffer*>& commandBuffers) = 0;
	virtual void Present() = 0;

	virtual CommandBuffer* GetCommandBuffer(CommandBufferType type = CommandBufferType::Primary, uint32_t threadIndex = 0) = 0;
};
