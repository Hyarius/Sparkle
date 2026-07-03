#pragma once

#include "board/cell_source.hpp"

namespace pg
{
	class LineOfSight
	{
	public:
		[[nodiscard]] static bool hasLine(
			const ICellSource &p_source,
			const spk::Vector3Int &p_fromCell,
			const spk::Vector3Int &p_toCell);
	};
}
