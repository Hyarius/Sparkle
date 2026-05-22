#pragma once

#if defined(SPARKLE_GPU_BACKEND_OPENGL)

#include "rendering/spk_viewport.hpp"

namespace spk::OpenGL
{
	class Viewport : public spk::Viewport
	{
	private:
		void _applyToGraphicsContext(const spk::ISurfaceState& p_surfaceState) const override;

	public:
		Viewport() = default;
		explicit Viewport(const spk::Rect2D& p_geometry);
		Viewport(const spk::Rect2D& p_geometry, const spk::Rect2D& p_scissor);
	};
}

#endif
