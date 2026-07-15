#pragma once

#include "battle/battle_command_result.hpp"
#include "battle/battle_phase.hpp"
#include "battle/battle_snapshot.hpp"

#include <array>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace pg
{
	struct ResourceViewModel
	{
		std::int64_t current = 0;
		std::int64_t maximum = 0;
		std::string label;
		[[nodiscard]] bool operator==(const ResourceViewModel &) const = default;
	};

	struct StatusViewModel
	{
		std::string id;
		std::string displayName;
		std::string description;
		std::optional<std::array<std::uint32_t, 2>> icon;
		std::uint32_t stacks = 0;
		std::string duration;
		[[nodiscard]] bool operator==(const StatusViewModel &) const = default;
	};

	// Complete copied data for a team-card inspection window.  This is deliberately presentation
	// data only: a frontend can render it freely without retaining a BattleUnit or querying rules.
	struct CreatureDetailsViewModel
	{
		ResourceViewModel health;
		ResourceViewModel actionPoints;
		ResourceViewModel movementPoints;
		std::int64_t strength = 0;
		std::int64_t armor = 0;
		std::int64_t magicPower = 0;
		std::int64_t resistance = 0;
		std::int64_t shield = 0;
		std::int64_t range = 0;
		BattleTime stamina{};
		std::vector<std::string> abilities;
		std::vector<StatusViewModel> passiveEffects;
		std::vector<StatusViewModel> activeEffects;
		[[nodiscard]] bool operator==(const CreatureDetailsViewModel &) const = default;
	};

	struct CreatureCardViewModel
	{
		std::optional<BattleUnitId> unit;
		std::optional<CreatureInstanceId> creature;
		BattleSide side = BattleSide::Player;
		std::string displayName;
		std::optional<std::array<std::uint32_t, 2>> icon;
		std::array<std::uint8_t, 4> tint{255, 255, 255, 255};
		ResourceViewModel health;
		std::int64_t shield = 0;
		BattleTime readyAt{};
		BattleTime readyIn{};
		bool placed = false;
		bool defeated = false;
		bool active = false;
		bool selectedForPlacement = false;
		CreatureDetailsViewModel details;
		[[nodiscard]] bool operator==(const CreatureCardViewModel &) const = default;
	};

	struct AbilitySlotViewModel
	{
		std::size_t shortcut = 0;
		std::optional<std::string> abilityId;
		std::string name;
		std::string description;
		std::string costText;
		std::string rangeText;
		std::optional<std::array<std::uint32_t, 2>> icon;
		std::vector<std::string> effects;
		bool selected = false;
		bool enabled = false;
		std::optional<CommandRejection> disabledReason;
		[[nodiscard]] bool operator==(const AbilitySlotViewModel &) const = default;
	};

	// The action bar is deliberately separate from the active-unit projection: during
	// deployment it presents the player creature selected for placement, whereas during an
	// activation it presents the unit whose turn is currently running.
	struct ActionBarViewModel
	{
		std::string displayName;
		BattleSide side = BattleSide::Player;
		ResourceViewModel health;
		ResourceViewModel actionPoints;
		ResourceViewModel movementPoints;
		std::vector<StatusViewModel> effects;
		[[nodiscard]] bool operator==(const ActionBarViewModel &) const = default;
	};

	struct TurnForecastEntryViewModel
	{
		BattleUnitId unit;
		std::string shortName;
		BattleSide side = BattleSide::Player;
		BattleTime readyAt{};
		BattleTime offsetFromNow{};
		[[nodiscard]] bool operator==(const TurnForecastEntryViewModel &) const = default;
	};

	struct ActiveUnitViewModel
	{
		BattleUnitId unit;
		std::string displayName;
		BattleSide side = BattleSide::Player;
		ResourceViewModel health;
		ResourceViewModel actionPoints;
		ResourceViewModel movementPoints;
		std::int64_t shield = 0;
		BattleTime stamina{};
		std::vector<StatusViewModel> statuses;
		std::string selectedAbilitySummary;
		[[nodiscard]] bool operator==(const ActiveUnitViewModel &) const = default;
	};

	struct DeploymentViewModel
	{
		std::size_t placedCount = 0;
		std::size_t requiredCount = 0;
		bool readyEnabled = false;
		std::optional<CommandRejection> readyDisabledReason;
		std::string instruction;
		[[nodiscard]] bool operator==(const DeploymentViewModel &) const = default;
	};

	struct BattleHudViewModel
	{
		BattlePhase phase = BattlePhase::Deployment;
		std::vector<CreatureCardViewModel> playerTeam;
		std::vector<CreatureCardViewModel> opponentTeam;
		std::vector<TurnForecastEntryViewModel> forecast;
		std::optional<ActiveUnitViewModel> activeUnit;
		std::optional<ActionBarViewModel> actionBar;
		std::array<AbilitySlotViewModel, 8> abilities{};
		DeploymentViewModel deployment;
		std::string statusLine;
		[[nodiscard]] bool operator==(const BattleHudViewModel &) const = default;
	};

	struct TurnForecastEntry
	{
		BattleUnitId unit;
		BattleTime readyAt{};
		[[nodiscard]] bool operator==(const TurnForecastEntry &) const = default;
	};

	[[nodiscard]] std::vector<TurnForecastEntry> forecastActivations(const BattleSnapshot &p_snapshot, std::size_t p_maximumEntries);
}
