#pragma once

#include <map>
#include <string>
#include <string_view>

namespace pg
{
	// The shared battle vocabulary. Every later step - JSON, events, effects, AI, Feats,
	// presentation - spells these concepts this way and no other, and reads their names from
	// the tables below rather than growing a private map of near-synonyms.
	//
	// Foundation only: naming a concept here does not mean its system exists yet.

	enum class BattleSide
	{
		Player,
		Enemy
	};

	enum class EncounterKind
	{
		Wild,
		Trainer,
		Gym,
		Special,
		Debug
	};

	enum class DamageKind
	{
		Physical,
		Magical
	};

	enum class BattleResource
	{
		ActionPoints,
		MovementPoints
	};

	// The cause of a current AP/MP pool change. Conditions and battle events share this
	// vocabulary so progression can filter the same transitions the log records.
	enum class ResourceChangeReason
	{
		AbilityCost,
		MovementCost,
		Effect,
		ActivationRefill,
		NextActivationPenaltyConsumption,
		EffectiveMaximumClamp
	};

	// The magical offense stat is magicPower - never ability, magic or spellPower. "Ability"
	// stays the name of an authored command definition. Range is a bonus that extends an
	// ability's maximum range and nothing else.
	//
	// Deliberately absent, and not to be added later without revisiting the design: accuracy,
	// evasion, critical chance, elemental types, initiative speed, mana, energy, armor or
	// resistance penetration, level, XP.
	enum class CreatureStat
	{
		MaxHealth,
		Strength,
		MagicPower,
		Armor,
		Resistance,
		MaxActionPoints,
		MaxMovementPoints,
		Stamina,
		Range
	};

	enum class BattleOutcome
	{
		Undecided,
		PlayerVictory,
		PlayerDefeat,
		Draw,
		Aborted
	};

	// JSON literals are lower camel case. Pass a table to JsonReader::requireEnum to parse a
	// value with a file/path diagnostic listing the accepted spellings.
	[[nodiscard]] const std::map<std::string, BattleSide> &battleSideNames();
	[[nodiscard]] const std::map<std::string, EncounterKind> &encounterKindNames();
	[[nodiscard]] const std::map<std::string, DamageKind> &damageKindNames();
	[[nodiscard]] const std::map<std::string, BattleResource> &battleResourceNames();
	[[nodiscard]] const std::map<std::string, CreatureStat> &creatureStatNames();
	[[nodiscard]] const std::map<std::string, BattleOutcome> &battleOutcomeNames();

	// The JSON spelling of a value, the exact inverse of the tables above.
	[[nodiscard]] std::string_view toString(BattleSide p_value);
	[[nodiscard]] std::string_view toString(EncounterKind p_value);
	[[nodiscard]] std::string_view toString(DamageKind p_value);
	[[nodiscard]] std::string_view toString(BattleResource p_value);
	[[nodiscard]] std::string_view toString(CreatureStat p_value);
	[[nodiscard]] std::string_view toString(BattleOutcome p_value);

	[[nodiscard]] BattleSide opposingSide(BattleSide p_side) noexcept;
}
