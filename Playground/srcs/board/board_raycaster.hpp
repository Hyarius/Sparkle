#pragma once

#include "world/world_raycaster.hpp"

#include <optional>

namespace pg
{
	class BoardData;

	class BoardRaycaster
	{
	public:
		[[nodiscard]] static std::optional<spk::Vector3Int> resolveMouseCell(
			const BoardData &p_board,
			const WorldRay &p_worldRay,
			float p_maxDistance = 1000.0f);
	};
}
