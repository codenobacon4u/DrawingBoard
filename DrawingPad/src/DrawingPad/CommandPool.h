#pragma once

#include "CommandBuffer.h"

namespace DrawingPad
{
	class CommandPool
	{
	public:
		CommandPool(GraphicsDevice* device)
		{}

		virtual ~CommandPool() {}

		virtual CommandBuffer* RequestCommandBuffer(CommandBufferType type = CommandBufferType::Primary) = 0;
		virtual void Reset() = 0;

	};
}
