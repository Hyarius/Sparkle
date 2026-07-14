#pragma once

#include "abilities/effect_definition.hpp"
#include "abilities/targeting_definition.hpp"
#include "core/json.hpp"
#include "structures/math/spk_vector2.hpp"

#include <cstdint>
#include <map>
#include <string>
#include <vector>

namespace pg
{
	// The moments a placed object reacts to a unit on its cell. unitLeftCell observes the unit
	// that just left, after the occupancy transition has committed; the object's anchor stays
	// its own cell.
	enum class BattleObjectTrigger
	{
		UnitEnteredCell,
		UnitLeftCell,
		UnitEndedMoveOnCell,
		UnitActivationStartedOnCell,
		UnitActivationEndedOnCell
	};

	struct BattleObjectTriggerSpec
	{
		std::string id;
		BattleObjectTrigger trigger = BattleObjectTrigger::UnitEnteredCell;
		TargetFilter targetFilter;
		// 0 means unlimited for the lifetime of the placed instance. A nonzero count is
		// exhausted once the instance has fired that many times.
		std::uint32_t maxTriggers = 0;
		bool removeWhenExhausted = false;
		std::vector<EffectApplication> effects;
	};

	// An immutable authored object - a trap, a ward, a wall. The placing effect authors how
	// long an instance lasts, so no duration is duplicated here, and nothing about a placed
	// instance (its creator, cell, remaining triggers) belongs in the definition either.
	//
	// Objects are not units: an object never makes a cell satisfy an occupant relation. A
	// blocking object cannot be placed on an occupied cell; step 10 locks runtime stacking.
	struct BattleObjectDefinition
	{
		std::string id;
		// Translation keys, not sentences: the text lives in resources/data/locales.
		std::string displayNameKey;
		std::string descriptionKey;
		spk::Vector2Int icon;
		std::vector<std::string> tags;
		bool blocksMovement = false;
		bool blocksLineOfSight = false;
		std::vector<BattleObjectTriggerSpec> triggers;
	};

	inline constexpr int BattleObjectSchemaVersion = 1;
	inline constexpr std::int64_t MaximumObjectTriggers = 1000000;

	[[nodiscard]] const std::map<std::string, BattleObjectTrigger> &battleObjectTriggerNames();

	// Leaves the id empty: the registry loader assigns the validated filename stem.
	[[nodiscard]] BattleObjectDefinition parseBattleObjectDefinition(JsonReader &p_reader);
}
