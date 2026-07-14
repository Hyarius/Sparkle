#pragma once

#include "feats/feat_board_definition.hpp"

#include <cstdint>
#include <map>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace pg
{
	// What a creature persists about one condition. Keys are the authored stable ids of step 03,
	// never array indices: an author may reorder a board's arrays without rewriting every save.
	//
	// Step 15/17 own evaluation and advancement; this file owns the shape they must write into,
	// because a persistent creature has to be saved and rebuilt before either exists.
	//
	// Only what survives a battle lives here. An incomplete "fight" condition keeps its count of
	// closed qualifying fight windows and its consecutive-fight streak, so "win two fights in a
	// row" works across battles; a "game" condition additionally keeps its open metric and its
	// per-id buckets. An open ability/turn/fight metric and an incomplete ability/turn window
	// count are battle-result temporaries and are simply absent here.
	struct PersistentConditionAdvancement
	{
		std::string conditionId;
		// Once true, never false again: a completed condition is not re-earned.
		bool completed = false;
		std::uint32_t qualifyingWindows = 0;
		std::uint32_t consecutiveWindowStreak = 0;
		std::optional<std::int64_t> gameMetric;
		std::map<std::string, std::int64_t> gameBuckets;

		[[nodiscard]] bool operator==(const PersistentConditionAdvancement &p_other) const = default;
	};

	// One top-level requirement of a node, plus every nested allOf/anyOf child, each with its own
	// entry. A composite is never collapsed into one scalar: "damage 10 or heal 10" has to say
	// which half is already done.
	struct FeatRequirementProgress
	{
		std::string requirementId;
		// The requirement itself first, then its children in definition order, depth first.
		std::vector<PersistentConditionAdvancement> conditionAdvancement;

		[[nodiscard]] bool operator==(const FeatRequirementProgress &p_other) const = default;
	};

	struct FeatNodeProgress
	{
		std::string nodeId;
		bool completed = false;
		std::vector<FeatRequirementProgress> requirements;

		[[nodiscard]] bool operator==(const FeatNodeProgress &p_other) const = default;
	};

	// The authoritative persistent half of a creature. Nodes are in board definition order, so
	// iteration, reward replay and summaries never depend on a hash order.
	struct FeatBoardProgress
	{
		std::string boardId;
		std::vector<FeatNodeProgress> nodes;
		// group id -> the one node that took it. Reconstructed from the completed nodes rather
		// than trusted, so a contradictory map cannot smuggle a second branch in.
		std::map<std::string, std::string> chosenExclusiveGroups;

		[[nodiscard]] const FeatNodeProgress *tryNode(std::string_view p_nodeId) const noexcept;
		[[nodiscard]] bool isCompleted(std::string_view p_nodeId) const noexcept;

		[[nodiscard]] bool operator==(const FeatBoardProgress &p_other) const = default;
	};

	// Every node and every condition of the board, the root completed and nothing else. The root
	// is implicit: it has no requirement, so it is complete the moment the creature exists.
	[[nodiscard]] FeatBoardProgress makeFreshFeatBoardProgress(const FeatBoardDefinition &p_board);

	// The same, with an authored set of already-completed nodes - what an encounter opponent is.
	// It does not pretend the enemy met the runtime requirements; it asserts that the author chose
	// a state the creature could legally have reached, and proves it:
	//
	//   * every id is a known node, listed once, and never the implicit root;
	//   * the completed set is connected to the root through completed nodes;
	//   * an order exists in which every minimumCompletedNodes and fromForm gate holds, replaying
	//     changeForm rewards as it goes (which is why the starting form is needed here);
	//   * at most one node of any exclusive group is completed.
	//
	// Throws std::invalid_argument, so the caller adds its own file/JSON path context.
	[[nodiscard]] FeatBoardProgress makePresetFeatBoardProgress(
		const FeatBoardDefinition &p_board,
		std::string_view p_startingFormId,
		std::span<const std::string> p_completedNodeIds);
}
