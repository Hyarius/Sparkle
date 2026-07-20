#pragma once

#include <cstdint>

namespace spk::SceneRenderPriorities
{
	// Shadow passes are created per cascade, hence this is a reserved priority range
	// rather than a single pass-specific value.
	inline constexpr std::int32_t ShadowBase = 100;
	inline constexpr std::int32_t MainOpaque = 1000;
	inline constexpr std::int32_t MainTransparent = 1100;
	inline constexpr std::int32_t MainOverlay = 1200;
}
