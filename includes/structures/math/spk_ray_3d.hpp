#pragma once

#include "structures/math/spk_vector3.hpp"

namespace spk
{
	struct Ray3D
	{
		spk::Vector3 origin{};
		spk::Vector3 direction{0.0f, 0.0f, 1.0f};

		[[nodiscard]] bool operator==(const Ray3D &) const noexcept = default;
	};
}
