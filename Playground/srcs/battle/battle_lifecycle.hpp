#pragma once

#include "battle/battle_event_log.hpp"
#include "battle/battle_ids.hpp"
#include "battle/battle_outcome_rules.hpp"
#include "encounters/encounter_resolver.hpp"
#include "voxel/voxel_enums.hpp"

#include "structures/math/spk_vector3.hpp"

#include <compare>
#include <cstdint>
#include <optional>
#include <string>

namespace pg
{
	// A fully value-owned request.  It is created before a mode transition is queued so retries
	// never reroll an encounter team or derive a different combat seed.
	struct BattleStartRequest
	{
		ResolvedEncounter encounter;
		spk::Vector3Int worldCell{};
		VoxelOrientation playerApproach = VoxelOrientation::PositiveZ;
		std::string stableEncounterInstanceId;
		std::uint64_t encounterOrdinal = 0;
		std::uint64_t encounterResolutionSeed = 0;
		std::uint64_t combatSeed = 0;
		std::optional<std::string> debugAutoplayPlayerProfileId;
	};

	struct BattleRuntimeGeneration
	{
		std::uint64_t value = 0;

		[[nodiscard]] bool valid() const noexcept
		{
			return value != 0;
		}

		[[nodiscard]] auto operator<=>(const BattleRuntimeGeneration &) const = default;
	};

	enum class BattleLifecycleStage
	{
		Entered,
		ResultReady,
		WillExit,
		Exited,
		EntryFailed
	};

	struct BattleLifecycleNotice
	{
		BattleLifecycleStage stage = BattleLifecycleStage::EntryFailed;
		BattleRuntimeGeneration generation;
		BattleId battleId;
		std::string encounterDefinitionId;
		std::optional<BattleOutcome> outcome;
	};

	struct BattleBatchPublication
	{
		BattleRuntimeGeneration generation;
		BattleId battleId;
		CommittedBattleBatch batch;
	};
}
