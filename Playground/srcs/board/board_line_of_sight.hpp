#pragma once

#include "board/board_data.hpp"

#include "structures/math/spk_vector3.hpp"

#include <optional>
#include <string_view>

namespace pg
{
	// The exact tag that makes a solid voxel see-through for targeting - a fence, a window, a
	// railing. It is a gameplay property of the voxel type, deliberately unrelated to render
	// transparency: a stained-glass wall may be see-through to the eye and opaque to a spell.
	inline constexpr std::string_view LineOfSightTransparentTag = "losTransparent";

	// The eye a unit sights along, above the walking height of the cell it stands on. A board query
	// constant in this slice, not JSON balance data: no ability tunes it.
	inline constexpr float DefaultBoardEyeHeight = 0.65f;

	struct LineOfSightResult
	{
		bool clear = false;
		// The first voxel that blocked, in traversal order. Board-local (the space the trace runs in).
		std::optional<spk::Vector3Int> firstBlockingVoxel;
	};

	// Blockers that are not terrain. Step 10 implements this over the battle objects standing on the
	// board: a blocksLineOfSight object maps to exactly one opaque voxel immediately above its support
	// cell, {support.x, support.y + 1, support.z}, and several blockers in that voxel are still one
	// boolean obstruction. The query is pure; BoardData never owns a definition pointer.
	class IBoardLineOfSightExtraBlockers
	{
	public:
		virtual ~IBoardLineOfSightExtraBlockers() = default;
		[[nodiscard]] virtual bool blocksVoxel(const spk::Vector3Int &p_boardLocalVoxel) const noexcept = 0;
	};

	// A deterministic 3D voxel query over ICellSource - so a synthetic fixture, a handcrafted arena
	// and a live field run identical rules. It is not a render raycast, not a physics query and not a
	// camera pick.
	//
	// The segment runs eye to eye, from the walking height of each support cell. A voxel blocks when
	// its definition is Solid and does not carry the losTransparent tag, or when the optional extra
	// blocker says so. Empty and Passable voxels never block by themselves - a unit sights over water
	// and through a bush.
	//
	// Voxels in either endpoint's own x/z column are ignored, for terrain and extras alike: a caster
	// standing next to its own wall, or on a slope whose upper half is in its column, does not blind
	// itself.
	//
	// The traversal is supercover Amanatides-Woo: when the ray crosses an edge or a corner at exactly
	// the same t, every newly touched cell is inspected in canonical X/Y/Z order before the ray moves
	// on, so a diagonal can never leak through the corner between two walls.
	//
	// Ability RANGE is not computed here. Step 08 measures x/z distance ignoring elevation, and calls
	// this only when the ability requires sight.
	class BoardLineOfSight
	{
	public:
		// Throws std::invalid_argument when either endpoint is not a standable cell of this board, and
		// std::runtime_error when the board's live terrain has gone stale.
		[[nodiscard]] static LineOfSightResult trace(
			const BoardData &p_board,
			BoardCell p_fromSupport,
			BoardCell p_toSupport,
			const IBoardLineOfSightExtraBlockers *p_extraBlockers = nullptr,
			float p_eyeHeight = DefaultBoardEyeHeight);
	};
}
