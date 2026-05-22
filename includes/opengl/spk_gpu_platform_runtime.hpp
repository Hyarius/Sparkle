#pragma once

#include <memory>
#include <stdexcept>
#include <string>
#include <typeinfo>

#include "rendering/spk_render_context.hpp"
#include "window/spk_frame.hpp"

namespace spk
{
	class IGPUPlatformRuntime
	{
	protected:
		template<typename TExpected>
		TExpected& requireFrame(IFrame& p_frame)
		{
			auto* ptr = dynamic_cast<TExpected*>(&p_frame);
			if (ptr == nullptr)
				throw std::runtime_error(
					std::string("IGPUPlatformRuntime: expected ") + typeid(TExpected).name() +
					", got " + typeid(p_frame).name());
			return *ptr;
		}

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

		// Blocks until the GPU backend has completed all previously submitted work for the
		// current context. This is intended for synchronization points such as screenshots and
		// pixel comparisons without exposing backend-specific calls to higher-level code.
		virtual void waitUntilWorkDone() = 0;
	};
}
