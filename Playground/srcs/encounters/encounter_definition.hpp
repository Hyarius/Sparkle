#pragma once

#include "battle/battle_types.hpp"
#include "board/handcrafted_battle_board_definition.hpp"
#include "core/definition_fields.hpp"
#include "core/json.hpp"
#include "core/registry.hpp"

#include <array>
#include <cstdint>
#include <string>
#include <variant>
#include <vector>

namespace pg
{
	enum class OpponentLineOrder
	{
		CenterOut,
		LeftToRight,
		RightToLeft
	};

	// Authored board-local [x, z] cells, in the order the enemy roster fills them.
	struct FixedOpponentPlacementPolicy
	{
		std::vector<std::array<int, 2>> cells;

		[[nodiscard]] bool operator==(const FixedOpponentPlacementPolicy &) const = default;
	};

	// The first eligible row that far from the enemy's outer edge; enumeration then walks row by
	// row toward the board centre and never back toward the edge. Within a row, a smaller
	// perpendicular coordinate is "left".
	struct ByLineOpponentPlacementPolicy
	{
		int rowsFromEnemyEdge = 0;
		OpponentLineOrder order = OpponentLineOrder::CenterOut;

		[[nodiscard]] bool operator==(const ByLineOpponentPlacementPolicy &) const noexcept = default;
	};

	// Without replacement, from the canonical legal enemy-zone cells, using the battle-local RNG:
	// never std::random_device, never a render-frame time.
	struct SeededRandomOpponentPlacementPolicy
	{
		[[nodiscard]] bool operator==(const SeededRandomOpponentPlacementPolicy &) const noexcept = default;
	};

	using OpponentPlacementPolicy = std::
		variant<FixedOpponentPlacementPolicy, ByLineOpponentPlacementPolicy, SeededRandomOpponentPlacementPolicy>;

	// The battle is an overlay on the world the encounter triggered in: the geometry comes from the
	// live voxels, so the definition supplies only the extent and the deployment depth. The
	// approach comes from the movement that triggered it, which is why this alternative has no
	// playerApproach.
	struct LiveWorldBoardPolicyDefinition
	{
		std::array<int, 2> size{11, 11};
		int deploymentDepth = 2;
		OpponentPlacementPolicy opponentPlacement;

		[[nodiscard]] bool operator==(const LiveWorldBoardPolicyDefinition &) const = default;
	};

	// The geometry, the extent and the approach all come from the referenced board definition, so
	// this alternative may not repeat any of them.
	struct HandcraftedBoardPolicyDefinition
	{
		std::string definitionId;
		OpponentPlacementPolicy opponentPlacement;

		[[nodiscard]] bool operator==(const HandcraftedBoardPolicyDefinition &) const = default;
	};

	using EncounterBoardPolicyDefinition =
		std::variant<LiveWorldBoardPolicyDefinition, HandcraftedBoardPolicyDefinition>;

	// An immutable recipe, not a creature: step 06 projects it through the ordinary derivation. No
	// enemy class exists and none will.
	struct EncounterSpawnDefinition
	{
		std::string id;
		std::string speciesId;
		std::string aiBehaviourId;
		std::vector<std::string> completedFeatNodeIds;
		DefinitionSource source;

		[[nodiscard]] bool operator==(const EncounterSpawnDefinition &) const = default;
	};

	struct EncounterTeamDefinition
	{
		std::string id;
		// A translation key, not a sentence (see resources/data/locales).
		std::string displayNameKey;
		std::uint64_t weight = 1;
		// Authored order is the enemy roster order, and therefore the selector tie-break order.
		std::vector<EncounterSpawnDefinition> members;

		[[nodiscard]] bool operator==(const EncounterTeamDefinition &) const = default;
	};

	// Tiers are sparse on purpose: an encounter authored for tiers 0 and 4 keeps serving its tier-0
	// teams until the player has four gym clears.
	struct EncounterTierDefinition
	{
		std::uint32_t tier = 0;
		std::vector<EncounterTeamDefinition> teams;

		[[nodiscard]] bool operator==(const EncounterTierDefinition &) const = default;
	};

	// What the world can throw at the player, and the only thing that can: there is no random
	// species table, no level range and no stat scaling anywhere in this file.
	//
	// Kind decides the rest of the schema, and the pairing is locked:
	//
	//     wild     live world, may tame, repeatable
	//     trainer  live world, no taming, cleared once
	//     gym      handcrafted, no taming, cleared once
	//     special  handcrafted, no taming, cleared once
	//     debug    either board, no taming, repeatable
	struct EncounterDefinition
	{
		std::string id;
		std::string displayNameKey;
		EncounterKind kind = EncounterKind::Wild;
		bool allowsTaming = false;
		bool repeatable = true;
		// Only a Bush-triggered wild encounter rolls it; a named encounter proceeds directly.
		std::uint32_t triggerChancePermille = 1000;
		EncounterBoardPolicyDefinition board;
		std::vector<EncounterTierDefinition> tiers;
		DefinitionSource source;

		[[nodiscard]] bool operator==(const EncounterDefinition &) const = default;
	};

	inline constexpr int EncounterSchemaVersion = 1;
	// Tier 9 is reserved for a postgame state that does not exist yet, so a cleared-gym count only
	// ever reaches 8. Both bounds live here, with the schema that validates against them.
	inline constexpr std::uint32_t MaximumEncounterTier = 9;
	inline constexpr std::uint32_t MaximumProgressionTier = MaximumEncounterTier;
	inline constexpr std::uint32_t MaximumClearedGymTier = 8;
	inline constexpr std::uint32_t MaximumTriggerChancePermille = 1000;
	inline constexpr std::uint64_t MinimumTeamWeight = 1;
	inline constexpr std::uint64_t MaximumTeamWeight = 1000000000;
	inline constexpr std::size_t MaximumTeamMembers = 6;

	[[nodiscard]] const std::map<std::string, OpponentLineOrder> &opponentLineOrderNames();

	// The board extent and deployment depth in force, whichever alternative was authored: a
	// handcrafted policy reads them from the definition it names.
	[[nodiscard]] std::array<int, 2> effectiveBoardSize(
		const EncounterBoardPolicyDefinition &p_board,
		const Registry<HandcraftedBattleBoardDefinition> &p_boards);
	[[nodiscard]] int effectiveDeploymentDepth(
		const EncounterBoardPolicyDefinition &p_board,
		const Registry<HandcraftedBattleBoardDefinition> &p_boards);
	[[nodiscard]] std::size_t largestTeamSize(const EncounterDefinition &p_encounter) noexcept;

	// Leaves the id empty: the registry loader assigns the validated filename stem. Everything
	// structural is proved here - kind/board pairing, taming and repeatability, tier order, weights,
	// team sizes, placement bounds against the effective board. The species, AI and completed-node
	// references stay plain ids and are resolved in core/combat_definition_validation.hpp, where the
	// spawn's derived loadout is also checked against the abilities its AI would cast.
	[[nodiscard]] EncounterDefinition parseEncounterDefinition(
		JsonReader &p_reader,
		const Registry<HandcraftedBattleBoardDefinition> &p_boards);
}
