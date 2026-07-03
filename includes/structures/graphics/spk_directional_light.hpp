#pragma once

#include <type_traits>

#include "structures/graphics/geometry/spk_color.hpp"
#include "structures/math/spk_vector3.hpp"
#include "type/spk_padding.hpp"

namespace spk
{
	// Directly matches a std140 block made of vec4 direction, vec4 color, and
	// vec4 ambient. Keep this type value-based: references would upload pointers.
	struct alignas(16) DirectionalLight
	{
		spk::Vector3 direction{};
		SPK_PADDING(1);
		spk::Color color{};
		float ambient = 0.0f;
		SPK_PADDING(3);
	};

	static_assert(sizeof(DirectionalLight) == 48);
	static_assert(alignof(DirectionalLight) == 16);
	static_assert(std::is_trivially_copyable_v<DirectionalLight>);
}
