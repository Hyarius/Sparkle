#pragma once

#include <string_view>

namespace spk::SceneRenderPasses
{
	inline constexpr std::string_view MainOpaque = "spk.scene.main_opaque";
	inline constexpr std::string_view MainTransparent = "spk.scene.main_transparent";
	inline constexpr std::string_view MainOverlay = "spk.scene.main_overlay";
}
