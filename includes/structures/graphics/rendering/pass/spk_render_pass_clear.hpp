#pragma once

#include <cstdint>
#include <optional>

#include "structures/graphics/geometry/spk_color.hpp"

namespace spk
{
	struct RenderPassClear
	{
		std::optional<spk::Color> color;
		std::optional<float> depth;
		std::optional<std::uint32_t> stencil;
	};
}
