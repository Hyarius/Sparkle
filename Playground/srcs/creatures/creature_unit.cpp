#include "creatures/creature_unit.hpp"

#include "core/registries.hpp"

#include <stdexcept>
#include <utility>

namespace pg
{
	namespace
	{
		struct ResolvedDefinitions
		{
			const CreatureSpeciesDefinition *species = nullptr;
			const FeatBoardDefinition *board = nullptr;
		};

		[[nodiscard]] ResolvedDefinitions resolve(const std::string &p_speciesId, const DerivationContext &p_context)
		{
			if (p_context.species == nullptr || p_context.featBoards == nullptr)
			{
				throw std::invalid_argument("derivation context is missing a registry");
			}

			ResolvedDefinitions result;
			result.species = p_context.species->tryGet(p_speciesId);
			if (result.species == nullptr)
			{
				throw std::invalid_argument("unknown species '" + p_speciesId + "'");
			}
			result.board = p_context.featBoards->tryGet(result.species->featBoardId);
			if (result.board == nullptr)
			{
				throw std::invalid_argument(
					"species '" + p_speciesId + "' selects unknown feat board '" + result.species->featBoardId + "'");
			}
			return result;
		}
	}

	CreatureUnit makeCreatureUnit(
		CreatureInstanceId p_id,
		std::string p_speciesId,
		std::span<const std::string> p_completedNodeIds,
		const DerivationContext &p_context)
	{
		if (!p_id.valid())
		{
			throw std::invalid_argument("a creature needs a valid instance id");
		}

		const ResolvedDefinitions definitions = resolve(p_speciesId, p_context);

		CreatureUnit result;
		result.id = p_id;
		result.speciesId = std::move(p_speciesId);
		// One path for both cases: an empty preset is a fresh creature, and it still goes through
		// the legality check rather than around it.
		result.featProgress = makePresetFeatBoardProgress(
			*definitions.board,
			definitions.species->defaultFormId,
			p_completedNodeIds);
		result.derived =
			deriveCreatureState(*definitions.species, *definitions.board, result.featProgress, p_context);
		return result;
	}

	CreatureUnit makeCreatureUnit(
		CreatureInstanceId p_id,
		std::string p_speciesId,
		std::span<const std::string> p_completedNodeIds,
		const Registries &p_registries)
	{
		return makeCreatureUnit(
			p_id,
			std::move(p_speciesId),
			p_completedNodeIds,
			derivationContextOf(p_registries));
	}

	void rebuildDerivedState(CreatureUnit &p_unit, const DerivationContext &p_context)
	{
		const ResolvedDefinitions definitions = resolve(p_unit.speciesId, p_context);
		// Assigned only once the new value exists: a failed rebuild leaves the old cache in place
		// rather than a half-derived one.
		DerivedCreatureState derived =
			deriveCreatureState(*definitions.species, *definitions.board, p_unit.featProgress, p_context);
		p_unit.derived = std::move(derived);
	}

	void rebuildDerivedState(CreatureUnit &p_unit, const Registries &p_registries)
	{
		rebuildDerivedState(p_unit, derivationContextOf(p_registries));
	}
}
