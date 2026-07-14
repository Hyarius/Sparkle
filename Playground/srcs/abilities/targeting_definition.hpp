#pragma once

#include "core/json.hpp"

#include <map>
#include <string>

namespace pg
{
	// Which relationships a cell or its occupant may have to the source for the cell to be
	// eligible. The same value answers two separate authored questions - "may this be the
	// anchor?" and "which captured cells does the effect keep?" - and an ability must answer
	// both. Friendly fire is never inferred from an enemy-only anchor.
	//
	// Relations are relative to the caster, or to a battle object's owning side:
	//   allowSelf       the source unit itself
	//   allowAllies     active units on the source's side, excluding self
	//   allowEnemies    active units on the opposing side
	//   allowDefeated   reserved for a future revive schema; must be false in every v1 file
	//   allowEmptyCell  an unoccupied cell. Battle objects are not units, so an object never
	//                   makes a cell satisfy an occupant relation
	struct TargetFilter
	{
		bool allowSelf = false;
		bool allowAllies = false;
		bool allowEnemies = false;
		bool allowDefeated = false;
		bool allowEmptyCell = false;

		[[nodiscard]] bool operator==(const TargetFilter &p_other) const noexcept = default;
	};

	// Locked x/z geometry, elevation never changing a distance:
	//   diamond         Manhattan distance abs(dx) + abs(dz)
	//   orthogonalLine  dx == 0 || dz == 0
	//   diagonalLine    abs(dx) == abs(dz)
	//   self            the anchor is the source cell
	enum class RangeShape
	{
		Self,
		Diamond,
		OrthogonalLine,
		DiagonalLine
	};

	struct RangeDefinition
	{
		RangeShape shape = RangeShape::Diamond;
		int minimum = 0;
		int maximum = 0;
		bool requiresLineOfSight = false;
	};

	// Area of effect around the chosen anchor:
	//   single    the anchor only
	//   diamond   Manhattan radius
	//   square    Chebyshev radius
	//   cross     the anchor plus cardinal rays up to radius
	//   line      a forward ray from the anchor along the dominant source-to-anchor axis
	enum class AreaShape
	{
		Single,
		Diamond,
		Square,
		Cross,
		Line
	};

	struct AreaDefinition
	{
		AreaShape shape = AreaShape::Single;
		int radius = 0;
	};

	inline constexpr int MaximumRangeDistance = 64;
	inline constexpr int MaximumAreaRadius = 16;

	[[nodiscard]] const std::map<std::string, RangeShape> &rangeShapeNames();
	[[nodiscard]] const std::map<std::string, AreaShape> &areaShapeNames();

	// Cells are not enumerated in this step; only the authored geometry is validated.
	[[nodiscard]] TargetFilter parseTargetFilter(JsonReader &p_reader);
	[[nodiscard]] RangeDefinition parseRangeDefinition(JsonReader &p_reader);
	[[nodiscard]] AreaDefinition parseAreaDefinition(JsonReader &p_reader);
}
