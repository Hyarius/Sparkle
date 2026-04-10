#pragma once

#include <memory>

#include "spk_render_context.hpp"

namespace spk
{
	class IFrame;

	class IGPUPlatformRuntime
	{
	public:
		virtual ~IGPUPlatformRuntime();

		virtual std::unique_ptr<IRenderContext> createRenderContext(IFrame& p_frame) = 0;
	};
}
