#pragma once

#include <memory>

#include "spk_rect_2d.hpp"
#include "spk_surface_state.hpp"

namespace spk
{
	class IRenderContext
	{
	private:
		std::shared_ptr<ISurfaceState> _surfaceState;

	protected:
		explicit IRenderContext(std::shared_ptr<ISurfaceState> p_surfaceState);

	public:
		virtual ~IRenderContext();

		// Invalidation is a lightweight cross-thread signal used when the native frame becomes
		// unavailable before the render thread has released the context. It must not destroy
		// GPU resources by itself; it only marks the context unusable until the render thread
		// performs the actual teardown.
		[[nodiscard]] std::shared_ptr<ISurfaceState> surfaceState() const;
		virtual void invalidate();
		[[nodiscard]] virtual bool isValid() const;

		virtual void makeCurrent() = 0;
		virtual void present() = 0;
		virtual void setVSync(bool p_enabled) = 0;
		virtual void notifyResize(const spk::Rect2D& p_rect) = 0;
	};
}
