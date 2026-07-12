#pragma once

#include "structures/graphics/rendering/state/spk_viewport.hpp"

namespace spk
{
	class FrameBufferObject;

	struct RenderTargetReference
	{
		const spk::FrameBufferObject *frameBuffer = nullptr;
		spk::Viewport viewport;
		// Compatibility contract for callers that deliberately render into the
		// framebuffer already selected by the surrounding capture/runtime.
		bool activeTarget = false;
	};
}
