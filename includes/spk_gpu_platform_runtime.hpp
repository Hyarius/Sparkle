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

		// The GPU runtime is expected to inspect the concrete frame implementation and downcast it
		// to the platform-specific frame type it supports. This is the intended extension point for
		// backends such as Win32, where render-context creation depends on native handles exposed by
		// the concrete frame implementation.
		//
		// The frame reference is provided as a source of backend-specific creation data only. GPU
		// backends should not treat it as a generic cross-thread control surface: once the render
		// context is created, platform-thread operations must still be routed through the frame owner.
		// The returned context is expected to be constructed directly with p_frame.surfaceState() so
		// it mirrors the lifetime of the native frame surface from birth.
		//
		// This method is expected to run on the render thread that will own the created context.
		virtual std::unique_ptr<IRenderContext> createRenderContext(IFrame& p_frame) = 0;
	};
}
