#pragma once

#include "structures/graphics/rendering/state/spk_viewport.hpp"

namespace spk
{
	class SurfaceState;

	class OpenGLViewport
	{
	public:
		static void apply(const spk::Viewport &p_viewport, const spk::SurfaceState &p_surfaceState);
	};
}
