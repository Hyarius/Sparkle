#pragma once

#include "structures/game_engine/spk_component.hpp"
#include "structures/math/spk_vector3.hpp"

#include <cstddef>
#include <vector>

namespace pg
{
	class BattleUnit;

	// Presentation state for one placed battle unit. The backend position remains authoritative;
	// this component only retains enough state to animate between resolved positions.
	class BattleUnitView : public spk::Component
	{
	public:
		BattleUnit *unit = nullptr;
		spk::Vector3Int displayedCell{};
		std::vector<spk::Vector3Int> path;
		std::size_t segment = 0;
		float segmentProgress = 0.0f;
		float sinkSeconds = 0.0f;
		bool sinking = false;
		bool hidden = false;

		BattleUnitView(BattleUnit &p_unit, const spk::Vector3Int &p_cell) :
			unit(&p_unit),
			displayedCell(p_cell)
		{
		}

		[[nodiscard]] bool moving() const noexcept
		{
			return segment + 1 < path.size();
		}
	};
}
