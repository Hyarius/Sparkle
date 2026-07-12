#pragma once

#include "structures/graphics/rendering/pass/spk_render_pass.hpp"

namespace spk::SceneRenderPasses
{
	inline constexpr auto MainOpaque = spk::makeRenderPassTypeId("spk.scene.main_opaque");
	inline constexpr auto MainTransparent = spk::makeRenderPassTypeId("spk.scene.main_transparent");
	inline constexpr auto MainOverlay = spk::makeRenderPassTypeId("spk.scene.main_overlay");
}
