#pragma once

#include "core/json.hpp"
#include "creatures/creature_state_derivation.hpp"
#include "player/player_data.hpp"

#include <string>
#include <vector>

namespace pg
{
	class Registries;

	// One authored starter. It carries no instance id: identity is the run's to allocate, not the
	// content's to hard-code, or two saves would own the same creature.
	struct NewGameCreatureDefinition
	{
		std::string speciesId;
		std::vector<std::string> completedFeatNodeIds;

		[[nodiscard]] bool operator==(const NewGameCreatureDefinition &p_other) const = default;
	};

	// The starting state of a run, authored rather than compiled in: main.cpp hard-codes no
	// starter. It is not a save file - step 18 defines those - and it is immutable content, so it
	// is loaded inside the same registry transaction as everything it references.
	struct NewGameDefinition
	{
		// Array order becomes team slot order, and the slots are filled left to right.
		std::vector<NewGameCreatureDefinition> starterTeam;
		DefinitionSource source;

		[[nodiscard]] bool operator==(const NewGameDefinition &p_other) const = default;
	};

	inline constexpr int NewGameSchemaVersion = 1;

	// Schema only: which species exist and which preset node sets are legal is resolved against the
	// registries, in makeNewPlayerData(), inside the load transaction.
	[[nodiscard]] NewGameDefinition parseNewGameDefinition(JsonReader &p_reader);

	// Creates the starters in authored order, so they take slots 0..N-1 and serials 1..N, and
	// leaves the storage box, the cleared sets and the encounter serial empty. Throws a JsonError
	// naming config/new-game.json and the offending entry when a species, a node preset or a
	// derived state does not hold.
	[[nodiscard]] PlayerData makeNewPlayerData(
		const NewGameDefinition &p_definition,
		const DerivationContext &p_context);

	[[nodiscard]] PlayerData makeNewPlayerData(
		const NewGameDefinition &p_definition,
		const Registries &p_registries);
}
