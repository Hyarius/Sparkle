#pragma once

#include "board/board_cell.hpp"

#include <cstdint>
#include <string_view>
#include <vector>

namespace pg
{
	enum class BattleMaskKind : std::uint8_t
	{
		Deployment,
		Reachable,
		Path,
		AbilityRange,
		AreaPreview,
		LineOfSightBlocked,
		Hovered,
		Invalid
	};

	[[nodiscard]] std::string_view overlayMaskKey(BattleMaskKind p_kind) noexcept;
	[[nodiscard]] int overlayMaskPriority(BattleMaskKind p_kind) noexcept;

	struct BattleMaskCell
	{
		BoardCell cell;
		BattleMaskKind kind = BattleMaskKind::Reachable;

		[[nodiscard]] bool operator==(const BattleMaskCell &p_other) const noexcept
		{
			return cell == p_other.cell && kind == p_other.kind;
		}
	};

	struct BattleOverlayModel
	{
		std::vector<BattleMaskCell> cells;
		std::uint64_t revision = 0;
	};

	// Resolves every coplanar category to one sorted semantic mask per support cell.  The model
	// owns no renderer and will not change revision for a mouse move that has no visible effect.
	class BattleOverlayModelBuilder
	{
	private:
		BattleOverlayModel _model;

	public:
		[[nodiscard]] const BattleOverlayModel &model() const noexcept;
		bool update(std::vector<BattleMaskCell> p_candidates);
		void clear();
	};
}
