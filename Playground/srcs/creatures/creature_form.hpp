#pragma once

#include "structures/math/spk_vector2.hpp"

#include <string>

namespace pg
{
	struct CreatureForm
	{
		std::string displayName;
		int tier = 0;
		std::string modelId;
		std::string animationSetId;
		spk::Vector2Int avatar{};
	};
}
