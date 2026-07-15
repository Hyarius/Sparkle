#pragma once

#include "board/traversal_graph.hpp"

#include "structures/math/spk_vector3.hpp"

#include <compare>
#include <cstdint>
#include <optional>
#include <string>

namespace pg
{
	// The canonical battle-board coordinate: the board-local SOLID SUPPORT voxel a unit stands
	// on, never the empty cell holding its feet. It is the same convention Actor::cell already
	// uses, so an exploration cell and a battle cell mean the same thing.
	//
	// It is a coordinate value, not a numeric identity - there is deliberately no BoardCellId.
	// A raw spk::Vector3Int stays right for a coordinate that is explicitly world- or
	// presentation-space; this alias marks the board-local space, and BoardData owns the only
	// conversions between the three. Every later command, plan, occupancy entry, event, snapshot
	// and HUD contract spells a board coordinate this way.
	using BoardCell = spk::Vector3Int;
	using BoardCellLess = CellPositionLess;

	enum class BoardSourceKind
	{
		// The board is a window onto the persistent voxel world exploration renders (decision
		// D03): a Wild or Trainer battle overlays terrain it never copies.
		LiveWorld,
		// The board owns an immutable grid: a Gym/Special arena materialised from a prefab, or a
		// test fixture. It has no world, no chunks, no streaming and no seed.
		Handcrafted
	};

	// Recorded explicitly on every board, never inferred from a zero anchor or from whether a
	// world pointer happens to be around: a handcrafted arena may not masquerade as live terrain
	// merely because its presentation origin is zero. Step 06 copies this value into the battle
	// descriptor and hashes it into the material digest.
	//
	// A LiveWorld board has no definition id. A Handcrafted one always has one: the registry id of
	// its HandcraftedBattleBoardDefinition, or - for a synthetic test fixture, which uses the same
	// owned-grid path - a stable explicit fixture id such as "test-flat-board" that is never
	// authored as production content. Any other combination is rejected at construction.
	struct BoardSourceDescriptor
	{
		BoardSourceKind kind = BoardSourceKind::LiveWorld;
		std::optional<std::string> definitionId;

		[[nodiscard]] auto operator<=>(const BoardSourceDescriptor &) const = default;
	};

	// Checked coordinate arithmetic. Every board origin addition/subtraction goes through these:
	// a coordinate that would wrap is nullopt, never a clamped or truncated cell.
	[[nodiscard]] std::optional<int> tryAdd(int p_left, int p_right) noexcept;
	[[nodiscard]] std::optional<int> trySubtract(int p_left, int p_right) noexcept;
	[[nodiscard]] std::optional<spk::Vector3Int> tryAdd(
		const spk::Vector3Int &p_left,
		const spk::Vector3Int &p_right) noexcept;
	[[nodiscard]] std::optional<spk::Vector3Int> trySubtract(
		const spk::Vector3Int &p_left,
		const spk::Vector3Int &p_right) noexcept;
}
