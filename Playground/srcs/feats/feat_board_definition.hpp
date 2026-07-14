#pragma once

#include "conditions/condition_definition.hpp"
#include "core/definition_fields.hpp"
#include "core/json.hpp"
#include "feats/feat_reward_definition.hpp"

#include <array>
#include <cstdint>
#include <functional>
#include <map>
#include <optional>
#include <set>
#include <string>
#include <string_view>
#include <vector>

namespace pg
{
	enum class FeatNodeKind
	{
		Root,
		Stats,
		Ability,
		Passive,
		Evolution
	};

	// One node of a creature's Feat Board. A node completes once and stays completed: there is
	// no repeatable node, no repeat limit and no repeat count - a stale one of those fails the
	// load as an unknown key.
	//
	// A node becomes active later (step 17) when it is incomplete, at least one neighbour is
	// completed, its minimumCompletedNodes and fromForm gates hold, and its exclusive group has
	// not already selected another node. Only nodes active at battle start consume that
	// battle's events.
	struct FeatNodeDefinition
	{
		std::string id;
		// Translation keys, not sentences (see resources/data/locales).
		std::string displayNameKey;
		std::string descriptionKey;
		// Board-space [x, y] for presentation. Overlaps are legal in v1: there is no editor yet.
		std::array<int, 2> position{};
		FeatNodeKind kind = FeatNodeKind::Stats;
		// Undirected: if A names B, B names A. The board is connected from its root.
		std::vector<std::string> neighbours;
		// Completing one member of a group permanently blocks every other member. Their
		// descendants stay locked through the adjacency and fromForm gates; respec and branch
		// reversal are out of scope.
		std::optional<std::string> exclusiveGroup;
		// Counts completed non-root nodes.
		std::uint32_t minimumCompletedNodes = 0;
		// Which species form the creature must currently have. Resolved in step 04, when a
		// species selects the board - a board on its own does not know what a form is.
		std::optional<std::string> fromForm;
		std::vector<ConditionSpec> requirements;
		std::vector<FeatRewardSpec> rewards;
		DefinitionSource source;
	};

	// The nodes vector order is authoritative: reward replay, simultaneous-completion tie-breaks
	// (earlier definition order wins), summaries and tests all read it. Lookup maps may be
	// derived from it but never replace it.
	struct FeatBoardDefinition
	{
		std::string id;
		std::string displayNameKey;
		std::string rootNodeId;
		std::vector<FeatNodeDefinition> nodes;
		DefinitionSource source;
	};

	inline constexpr int FeatBoardSchemaVersion = 1;
	inline constexpr int MaximumFeatNodeCoordinate = 4096;

	[[nodiscard]] const std::map<std::string, FeatNodeKind> &featNodeKindNames();

	// Leaves the id empty: the registry loader assigns the validated filename stem. Parses and
	// validates everything a board can prove on its own - schema, ids, graph, gates, exclusive
	// groups - and leaves two things to later phases: ability/status references (resolved in
	// core/combat_definition_validation.hpp, once those registries exist) and form references
	// (resolved in step 04, once a species selects this board).
	[[nodiscard]] FeatBoardDefinition parseFeatBoardDefinition(JsonReader &p_reader, const ConditionLimits &p_limits);

	// The deferred half of the form contract, and the reason every node and reward kept its
	// authored file and JSON path: step 04 calls this with the selecting species' form ids, and
	// a failure still points at the board file the author has to fix.
	void validateFeatBoardFormReferences(
		const FeatBoardDefinition &p_board,
		const std::set<std::string, std::less<>> &p_formIds,
		std::string_view p_speciesId);

	// Every form id the board names, whether as a fromForm gate or as a changeForm reward.
	[[nodiscard]] std::set<std::string> featBoardFormReferences(const FeatBoardDefinition &p_board);
}
