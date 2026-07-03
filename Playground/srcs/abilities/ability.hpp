#pragma once

#include "battle/rules/battle_geometry.hpp"

#include <array>
#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace pg
{
	class JsonReader;
	class Effect;
	struct Status;
	template <typename TDefinition>
	class Registry;
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
	enum class TravelVfxKind
	{
		Projectile,
		Beam
	};
	struct TravelVfx
	{
		TravelVfxKind kind = TravelVfxKind::Projectile;
		std::string vfx;
		float speed = 0.0f;
		float duration = 0.0f;
	};

	struct Ability
	{
		std::string id;
		std::string displayName;
		std::array<int, 2> icon{};
		int apCost = 0, mpCost = 0;
		int minimumRange = 1, maximumRange = 1;
		RangeShape rangeShape = RangeShape::Circle;
		bool requiresLineOfSight = true;
		// Area of effect around the resolved anchor cell. value 0 = single cell (step 11 scope:
		// tackle is single-target; the geometry is still exercised by previews + truth tables).
		AreaShape areaShape = AreaShape::Square;
		int areaValue = 0;
		TargetProfile targetProfile = TargetProfile::Enemy;
		std::string casterAnimation;
		std::string targetAnimation = "TakeDamage";
		std::optional<TravelVfx> travelVfx;
		std::vector<std::shared_ptr<const Effect>> effects;

		// Kept as a source-compatible bridge for step-10 callers that construct abilities in
		// code. Authored abilities use effects; the resolver only falls back to these fields
		// when effects is empty.
		int baseDamage = 0;
		DamageKind damageKind = DamageKind::Physical;
		float attackRatio = 0.0f, magicRatio = 0.0f;
	};

	[[nodiscard]] Ability parseAbility(JsonReader &p_reader);
	[[nodiscard]] Ability parseAbility(JsonReader &p_reader, const Registry<Status> &p_statuses);
}
