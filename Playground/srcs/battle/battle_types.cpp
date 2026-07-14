#include "battle/battle_types.hpp"

#include <stdexcept>

namespace pg
{
	namespace
	{
		// One table per enum, and toString() reads it backwards, so a spelling cannot drift
		// between the parser and the writer.
		template <typename TEnum>
		[[nodiscard]] std::string_view nameOf(const std::map<std::string, TEnum> &p_names, TEnum p_value)
		{
			for (const auto &[name, value] : p_names)
			{
				if (value == p_value)
				{
					return name;
				}
			}
			throw std::invalid_argument("battle enum value has no JSON spelling");
		}
	}

	const std::map<std::string, BattleSide> &battleSideNames()
	{
		static const std::map<std::string, BattleSide> names{
			{"player", BattleSide::Player},
			{"enemy", BattleSide::Enemy}};
		return names;
	}

	const std::map<std::string, EncounterKind> &encounterKindNames()
	{
		static const std::map<std::string, EncounterKind> names{
			{"wild", EncounterKind::Wild},
			{"trainer", EncounterKind::Trainer},
			{"gym", EncounterKind::Gym},
			{"special", EncounterKind::Special},
			{"debug", EncounterKind::Debug}};
		return names;
	}

	const std::map<std::string, DamageKind> &damageKindNames()
	{
		static const std::map<std::string, DamageKind> names{
			{"physical", DamageKind::Physical},
			{"magical", DamageKind::Magical}};
		return names;
	}

	const std::map<std::string, BattleResource> &battleResourceNames()
	{
		static const std::map<std::string, BattleResource> names{
			{"actionPoints", BattleResource::ActionPoints},
			{"movementPoints", BattleResource::MovementPoints}};
		return names;
	}

	const std::map<std::string, CreatureStat> &creatureStatNames()
	{
		static const std::map<std::string, CreatureStat> names{
			{"maxHealth", CreatureStat::MaxHealth},
			{"strength", CreatureStat::Strength},
			{"magicPower", CreatureStat::MagicPower},
			{"armor", CreatureStat::Armor},
			{"resistance", CreatureStat::Resistance},
			{"maxActionPoints", CreatureStat::MaxActionPoints},
			{"maxMovementPoints", CreatureStat::MaxMovementPoints},
			{"stamina", CreatureStat::Stamina},
			{"range", CreatureStat::Range}};
		return names;
	}

	const std::map<std::string, BattleOutcome> &battleOutcomeNames()
	{
		static const std::map<std::string, BattleOutcome> names{
			{"undecided", BattleOutcome::Undecided},
			{"playerVictory", BattleOutcome::PlayerVictory},
			{"playerDefeat", BattleOutcome::PlayerDefeat},
			{"draw", BattleOutcome::Draw},
			{"aborted", BattleOutcome::Aborted}};
		return names;
	}

	std::string_view toString(BattleSide p_value)
	{
		return nameOf(battleSideNames(), p_value);
	}

	std::string_view toString(EncounterKind p_value)
	{
		return nameOf(encounterKindNames(), p_value);
	}

	std::string_view toString(DamageKind p_value)
	{
		return nameOf(damageKindNames(), p_value);
	}

	std::string_view toString(BattleResource p_value)
	{
		return nameOf(battleResourceNames(), p_value);
	}

	std::string_view toString(CreatureStat p_value)
	{
		return nameOf(creatureStatNames(), p_value);
	}

	std::string_view toString(BattleOutcome p_value)
	{
		return nameOf(battleOutcomeNames(), p_value);
	}

	BattleSide opposingSide(BattleSide p_side) noexcept
	{
		return p_side == BattleSide::Player ? BattleSide::Enemy : BattleSide::Player;
	}
}
