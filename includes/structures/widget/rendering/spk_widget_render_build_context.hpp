#pragma once

#include <cstddef>

#include "structures/graphics/rendering/pass/spk_render_pass.hpp"
#include "structures/graphics/rendering/spk_render_frame_build_context.hpp"

namespace spk
{
	struct WidgetRenderBuildContext
	{
		spk::RenderFrameBuildContext &frame;
		spk::RenderPass::ScopeId widgetScope;
		std::size_t *nextRegistrationOrder = nullptr;
	};
}
