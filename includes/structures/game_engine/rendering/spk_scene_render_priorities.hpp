#pragma once

#include <cstdint>

namespace spk::SceneRenderPriorities
{
	inline constexpr std::int32_t ShadowBase = 100;
	inline constexpr std::int32_t MainOpaque = 1000;
	inline constexpr std::int32_t MainTransparent = 1100;
	inline constexpr std::int32_t MainOverlay = 1200;
	inline constexpr std::int32_t PostProcessingBase = 2000;
}
