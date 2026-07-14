#include "abilities/targeting_definition.hpp"

#include "core/definition_fields.hpp"

namespace pg
{
	const std::map<std::string, RangeShape> &rangeShapeNames()
	{
		static const std::map<std::string, RangeShape> names{
			{"diagonalLine", RangeShape::DiagonalLine},
			{"diamond", RangeShape::Diamond},
			{"orthogonalLine", RangeShape::OrthogonalLine},
			{"self", RangeShape::Self}};
		return names;
	}

	const std::map<std::string, AreaShape> &areaShapeNames()
	{
		static const std::map<std::string, AreaShape> names{
			{"cross", AreaShape::Cross},
			{"diamond", AreaShape::Diamond},
			{"line", AreaShape::Line},
			{"single", AreaShape::Single},
			{"square", AreaShape::Square}};
		return names;
	}

	TargetFilter parseTargetFilter(JsonReader &p_reader)
	{
		p_reader.forbidUnknown({"allowSelf", "allowAllies", "allowEnemies", "allowDefeated", "allowEmptyCell"});

		TargetFilter result;
		result.allowSelf = p_reader.require<bool>("allowSelf");
		result.allowAllies = p_reader.require<bool>("allowAllies");
		result.allowEnemies = p_reader.require<bool>("allowEnemies");
		result.allowDefeated = p_reader.require<bool>("allowDefeated");
		result.allowEmptyCell = p_reader.require<bool>("allowEmptyCell");

		if (result.allowDefeated)
		{
			throw JsonError(
				p_reader.file(),
				p_reader.pathFor("allowDefeated"),
				"allowDefeated is reserved for a future revive schema and must be false in a v1 definition");
		}
		if (!result.allowSelf && !result.allowAllies && !result.allowEnemies && !result.allowEmptyCell)
		{
			throw JsonError(
				p_reader.file(),
				p_reader.path(),
				"a filter must allow at least one of allowSelf, allowAllies, allowEnemies or allowEmptyCell");
		}
		return result;
	}

	RangeDefinition parseRangeDefinition(JsonReader &p_reader)
	{
		p_reader.forbidUnknown({"shape", "minimum", "maximum", "requiresLineOfSight"});

		RangeDefinition result;
		result.shape = p_reader.requireEnum<RangeShape>("shape", rangeShapeNames());
		result.minimum = static_cast<int>(requireIntegerInRange(p_reader, "minimum", 0, MaximumRangeDistance));
		result.maximum = static_cast<int>(requireIntegerInRange(p_reader, "maximum", 0, MaximumRangeDistance));
		result.requiresLineOfSight = p_reader.require<bool>("requiresLineOfSight");

		if (result.minimum > result.maximum)
		{
			throw JsonError(
				p_reader.file(),
				p_reader.pathFor("minimum"),
				"minimum range " + std::to_string(result.minimum) + " exceeds maximum range " +
					std::to_string(result.maximum));
		}
		if (result.shape == RangeShape::Self && (result.minimum != 0 || result.maximum != 0))
		{
			throw JsonError(
				p_reader.file(),
				p_reader.path(),
				"a self range anchors on the source cell, so both minimum and maximum must be zero");
		}
		if (result.shape != RangeShape::Self && result.maximum == 0)
		{
			throw JsonError(
				p_reader.file(),
				p_reader.pathFor("maximum"),
				"a non-self range must reach at least one cell; author shape 'self' instead");
		}
		return result;
	}

	AreaDefinition parseAreaDefinition(JsonReader &p_reader)
	{
		p_reader.forbidUnknown({"shape", "radius"});

		AreaDefinition result;
		result.shape = p_reader.requireEnum<AreaShape>("shape", areaShapeNames());
		result.radius = static_cast<int>(requireIntegerInRange(p_reader, "radius", 0, MaximumAreaRadius));

		// The other shapes accept radius zero as a degenerate single cell; "single" may not
		// carry a radius at all, so an authored radius on it is always a mistake.
		if (result.shape == AreaShape::Single && result.radius != 0)
		{
			throw JsonError(
				p_reader.file(),
				p_reader.pathFor("radius"),
				"a single-cell area has no radius, got " + std::to_string(result.radius));
		}
		return result;
	}
}
