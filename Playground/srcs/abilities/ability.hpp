#pragma once

#include "battle/rules/battle_geometry.hpp"

#include <string>

namespace pg
{
	class JsonReader;
	enum class DamageKind
	{
		Physical,
		Magical
	};
	enum class TargetProfile
	{
		Everything,
		Ally,
		Enemy,
		Empty
	};

	struct Ability
	{
		std::string id;
		std::string displayName;
		int apCost = 0, mpCost = 0;
		int minimumRange = 1, maximumRange = 1;
		RangeShape rangeShape = RangeShape::Circle;
		bool requiresLineOfSight = true;
		// Area of effect around the resolved anchor cell. value 0 = single cell (step 11 scope:
		// tackle is single-target; the geometry is still exercised by previews + truth tables).
		AreaShape areaShape = AreaShape::Square;
		int areaValue = 0;
		TargetProfile targetProfile = TargetProfile::Enemy;
		int baseDamage = 0;
		DamageKind damageKind = DamageKind::Physical;
		float attackRatio = 0.0f, magicRatio = 0.0f;
	};

	[[nodiscard]] Ability parseAbility(JsonReader &p_reader);
}
