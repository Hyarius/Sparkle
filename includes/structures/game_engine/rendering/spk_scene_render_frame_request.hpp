#pragma once

#include "structures/graphics/rendering/pass/spk_render_pass_clear.hpp"
#include "structures/graphics/rendering/pass/spk_render_target_reference.hpp"

namespace spk
{
	struct SceneRenderFrameRequest
	{
		spk::RenderTargetReference mainTarget;
		spk::RenderPassClear mainClear;
	};
}
