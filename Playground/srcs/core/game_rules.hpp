#pragma once

#include "battle/battle_time.hpp"
#include "core/json.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <map>
#include <string>

namespace pg
{
	// The structural limits and tunables the battle steps share. Structural values are
	// serialized invariants, not free tunables: the loadout array, the AI enumeration and the
	// eight-slot HUD are all sized in code, so a rules file that disagrees with them is a bug
	// and must fail loudly at startup rather than half-work.
	struct BattleGameRules
	{
		static constexpr std::size_t RequiredTeamCapacity = 6;
		static constexpr std::size_t RequiredAbilitySlotCapacity = 8;

		static constexpr int MinimumBoardSide = 5;
		static constexpr int MaximumBoardSide = 31;
		static constexpr std::int64_t MinimumMitigationScale = 1;
		static constexpr std::int64_t MaximumMitigationScale = 1000000;
		static constexpr std::size_t MaximumEffectChainDepth = 256;
		static constexpr std::size_t MaximumConditionDepth = 32;

		std::size_t teamCapacity = RequiredTeamCapacity;
		std::size_t abilitySlotCapacity = RequiredAbilitySlotCapacity;
		// Width (x) then depth (z), both odd so the board has a single centre column/row.
		std::array<int, 2> defaultBoardSize{11, 11};
		int deploymentDepth = 2;
		BattleTime minimumStamina = BattleTime::fromTicks(100);
		std::int64_t mitigationScale = 100;
		std::size_t maxCommandsPerActivation = 64;
		std::size_t maxAiCommandsPerActivation = 32;
		std::size_t maxEffectChainDepth = 32;
		std::size_t maxConditionDepth = 8;
	};

	struct GameRules
	{
		double maxVerticalTraversalGap = 0.0;
		BattleGameRules battle;
		// Overlay category -> mask atlas cell. The eight keys are fixed by the schema.
		std::map<std::string, std::array<int, 2>> overlayMasks;
	};

	// Throws JsonError, naming the file, the JSON path, the bad value and the constraint.
	[[nodiscard]] GameRules parseGameRules(JsonReader &p_reader);
}
