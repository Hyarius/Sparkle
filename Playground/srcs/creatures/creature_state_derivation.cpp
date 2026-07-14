#include "creatures/creature_state_derivation.hpp"

#include "core/registries.hpp"

#include <algorithm>
#include <set>
#include <stdexcept>
#include <variant>

namespace pg
{
	namespace
	{
		[[noreturn]] void failReward(
			const CreatureSpeciesDefinition &p_species,
			const FeatBoardDefinition &p_board,
			const FeatNodeDefinition &p_node,
			const FeatRewardSpec &p_reward,
			const std::string &p_message)
		{
			throw JsonError(
				p_reward.source.file,
				p_reward.source.jsonPath,
				"species '" + p_species.id + "' on board '" + p_board.id + "': node '" + p_node.id + "' reward '" +
					p_reward.id + "' " + p_message);
		}

		void applyReward(
			const CreatureSpeciesDefinition &p_species,
			const FeatBoardDefinition &p_board,
			const FeatNodeDefinition &p_node,
			const FeatRewardSpec &p_reward,
			DerivedCreatureState &p_state)
		{
			if (const auto *bonus = std::get_if<BonusStatRewardSpec>(&p_reward.payload))
			{
				try
				{
					addToAttribute(p_state.attributes, bonus->stat, bonus->amount, bonus->staminaDelta);
				} catch (const std::overflow_error &error)
				{
					failReward(p_species, p_board, p_node, p_reward, error.what());
				}
			}
			else if (const auto *unlockAbility = std::get_if<UnlockAbilityRewardSpec>(&p_reward.payload))
			{
				// Append only when absent: replaying the same board twice must not grow the list,
				// and an ability unlocked twice by two branches still occupies one slot.
				if (std::ranges::find(p_state.abilityIds, unlockAbility->ability) == p_state.abilityIds.end())
				{
					p_state.abilityIds.push_back(unlockAbility->ability);
				}
			}
			else if (const auto *removeAbility = std::get_if<RemoveAbilityRewardSpec>(&p_reward.payload))
			{
				std::erase(p_state.abilityIds, removeAbility->ability);
			}
			else if (const auto *unlockPassive = std::get_if<UnlockPassiveRewardSpec>(&p_reward.payload))
			{
				if (std::ranges::find(p_state.passiveStatusIds, unlockPassive->status) == p_state.passiveStatusIds.end())
				{
					p_state.passiveStatusIds.push_back(unlockPassive->status);
				}
			}
			else if (const auto *changeForm = std::get_if<ChangeFormRewardSpec>(&p_reward.payload))
			{
				p_state.formId = changeForm->form;
			}
		}

		void requireContext(const DerivationContext &p_context)
		{
			if (p_context.gameRules == nullptr || p_context.statuses == nullptr || p_context.abilities == nullptr ||
				p_context.featBoards == nullptr || p_context.species == nullptr)
			{
				throw std::invalid_argument("derivation context is missing a registry");
			}
		}

		void validateDerived(
			const CreatureSpeciesDefinition &p_species,
			const DerivedCreatureState &p_state,
			const DerivationContext &p_context)
		{
			const std::string owner = "species '" + p_species.id + "'";
			requireValidAttributes(p_state.attributes, *p_context.gameRules, p_species.source, owner);

			if (p_state.abilityIds.empty())
			{
				throw JsonError(
					p_species.source.file,
					p_species.source.jsonPath,
					owner + " derives no ability at all; a creature that cannot act cannot fight");
			}
			if (p_state.abilityIds.size() > p_context.gameRules->battle.abilitySlotCapacity)
			{
				throw JsonError(
					p_species.source.file,
					p_species.source.jsonPath,
					owner + " derives " + std::to_string(p_state.abilityIds.size()) +
						" abilities, over the " + std::to_string(p_context.gameRules->battle.abilitySlotCapacity) +
						" loadout slots");
			}
			for (const std::string &abilityId : p_state.abilityIds)
			{
				if (!p_context.abilities->contains(abilityId))
				{
					throw JsonError(
						p_species.source.file, p_species.source.jsonPath, owner + " derives unknown ability '" + abilityId + "'");
				}
			}
			for (const std::string &statusId : p_state.passiveStatusIds)
			{
				const StatusDefinition *status = p_context.statuses->tryGet(statusId);
				if (status == nullptr)
				{
					throw JsonError(
						p_species.source.file, p_species.source.jsonPath, owner + " derives unknown passive '" + statusId + "'");
				}
				if (isStunStatus(*status))
				{
					throw JsonError(
						p_species.source.file,
						p_species.source.jsonPath,
						owner + " derives the stun status '" + statusId + "' as a permanent passive");
				}
			}
			if (p_species.tryForm(p_state.formId) == nullptr)
			{
				throw JsonError(
					p_species.source.file,
					p_species.source.jsonPath,
					owner + " derives form '" + p_state.formId + "', which it does not define");
			}
		}
	}

	DerivationContext derivationContextOf(const Registries &p_registries)
	{
		return DerivationContext{
			.gameRules = &p_registries.gameRules(),
			.statuses = &p_registries.statuses(),
			.abilities = &p_registries.abilities(),
			.featBoards = &p_registries.featBoards(),
			.species = &p_registries.species()};
	}

	DerivedCreatureState deriveCreatureState(
		const CreatureSpeciesDefinition &p_species,
		const FeatBoardDefinition &p_board,
		const FeatBoardProgress &p_progress,
		const DerivationContext &p_context)
	{
		requireContext(p_context);

		if (p_progress.boardId != p_board.id || p_species.featBoardId != p_board.id)
		{
			throw std::invalid_argument(
				"species '" + p_species.id + "' derives against board '" + p_board.id + "', but its progress records '" +
				p_progress.boardId + "' and its species records '" + p_species.featBoardId + "'");
		}

		DerivedCreatureState result;
		result.attributes = p_species.baseAttributes;
		result.abilityIds = p_species.defaultAbilityIds;
		result.passiveStatusIds = p_species.defaultPassiveStatusIds;
		result.formId = p_species.defaultFormId;

		// Gathered once. The replay itself still walks the board, so the completion set decides
		// which nodes apply and never in what order they do.
		std::set<std::string, std::less<>> completed;
		for (const FeatNodeProgress &node : p_progress.nodes)
		{
			if (node.completed)
			{
				completed.insert(node.nodeId);
			}
		}

		// Board definition order, then node reward order. Nothing else: not progress order, not
		// completion order, and never a hash order.
		for (const FeatNodeDefinition &node : p_board.nodes)
		{
			if (!completed.contains(node.id))
			{
				continue;
			}
			for (const FeatRewardSpec &reward : node.rewards)
			{
				applyReward(p_species, p_board, node, reward, result);
			}
		}

		validateDerived(p_species, result, p_context);
		return result;
	}

	DerivedCreatureState deriveCreatureState(
		const CreatureSpeciesDefinition &p_species,
		const FeatBoardDefinition &p_board,
		const FeatBoardProgress &p_progress,
		const Registries &p_registries)
	{
		return deriveCreatureState(p_species, p_board, p_progress, derivationContextOf(p_registries));
	}
}
