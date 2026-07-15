#pragma once

#include "battle/presentation/battle_presentation_cell_source.hpp"

#include "structures/math/spk_ray_3d.hpp"

#include <optional>

namespace pg
{
	struct BattlePick
	{
		BoardCell boardCell;
		spk::Vector3Int presentationSupportCell;
		spk::Vector3 cellEntryPosition;
		float rayDistance = 0.0f;
	};

	class BattleBoardPicker
	{
	public:
		[[nodiscard]] std::optional<BattlePick> pick(
			const BoardData &p_board,
			const IBattlePresentationCellSource &p_presentationCells,
			const spk::Ray3D &p_ray,
			float p_maxDistance = 1000.0f) const;
	};
}
