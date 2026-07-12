#pragma once

#include <memory>

#include "structures/graphics/rendering/state/spk_viewport.hpp"

namespace spk
{
	class FrameBufferObject;

	struct RenderTargetReference
	{
		// Keeps an off-screen target alive after its pass has been lowered into a
		// queued RenderUnit. Raw-pointer callers remain supported.
		std::shared_ptr<const spk::FrameBufferObject> ownedFrameBuffer;
		const spk::FrameBufferObject *frameBuffer = nullptr;
		spk::Viewport viewport;
		// Compatibility contract for callers that deliberately render into the
		// framebuffer already selected by the surrounding capture/runtime.
		bool activeTarget = false;
	};
}
