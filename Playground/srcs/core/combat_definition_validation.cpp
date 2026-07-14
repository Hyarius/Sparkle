#include "core/combat_definition_validation.hpp"

#include "creatures/creature_unit.hpp"
#include "feats/feat_board_progress.hpp"

#include <algorithm>
#include <exception>
#include <limits>
#include <map>
#include <set>
#include <stdexcept>
#include <string>
#include <variant>
#include <vector>

namespace pg
{
	namespace
	{
		// The effect kept its authored file and JSON path through phase 1 precisely so this
		// message can point at the line the author has to fix, rather than at the registry.
		[[noreturn]] void fail(
			const std::string &p_owner,
			const EffectApplication &p_effect,
			const std::string &p_message)
		{
			throw JsonError(
				p_effect.source.file,
				p_effect.source.jsonPath,
				"definition '" + p_owner + "' effect '" + p_effect.id + "' " + p_message);
		}

		[[nodiscard]] const StatusDefinition &requireStatus(
			const Registry<StatusDefinition> &p_statuses,
			const std::string &p_owner,
			const EffectApplication &p_effect,
			const std::string &p_statusId)
		{
			const StatusDefinition *status = p_statuses.tryGet(p_statusId);
			if (status == nullptr)
			{
				fail(p_owner, p_effect, "references unknown status '" + p_statusId + "'");
			}
			return *status;
		}

		void requireBattleObject(
			const Registry<BattleObjectDefinition> &p_battleObjects,
			const std::string &p_owner,
			const EffectApplication &p_effect,
			const std::string &p_objectId)
		{
			if (!p_battleObjects.contains(p_objectId))
			{
				fail(p_owner, p_effect, "references unknown battle object '" + p_objectId + "'");
			}
		}

		void validateEffect(
			const Registry<StatusDefinition> &p_statuses,
			const Registry<BattleObjectDefinition> &p_battleObjects,
			const std::string &p_owner,
			const EffectApplication &p_effect)
		{
			if (const auto *applyStatus = std::get_if<ApplyStatusEffectSpec>(&p_effect.payload))
			{
				const StatusDefinition &status =
					requireStatus(p_statuses, p_owner, p_effect, applyStatus->status);

				// A stunned unit cannot reliably consume its own turn to expire the stun, so an
				// owner-activation stun could last forever; an infinite one always would.
				if (isStunStatus(status) && !std::holds_alternative<TimelineDurationSpec>(applyStatus->duration))
				{
					fail(
						p_owner,
						p_effect,
						"applies the stun status '" + applyStatus->status +
							"', which requires a finite timeline duration");
				}
			}
			else if (const auto *removeStatus = std::get_if<RemoveStatusEffectSpec>(&p_effect.payload))
			{
				(void)requireStatus(p_statuses, p_owner, p_effect, removeStatus->status);
			}
			else if (const auto *placeObject = std::get_if<PlaceObjectEffectSpec>(&p_effect.payload))
			{
				requireBattleObject(p_battleObjects, p_owner, p_effect, placeObject->object);
			}

			// cleanse and removeObjects select by tag, and a tag that matches nothing today is
			// legitimate: later content may introduce it. Their grammar was checked in phase 1.
		}

		void validateEffects(
			const Registry<StatusDefinition> &p_statuses,
			const Registry<BattleObjectDefinition> &p_battleObjects,
			const std::string &p_owner,
			const std::vector<EffectApplication> &p_effects)
		{
			for (const EffectApplication &effect : p_effects)
			{
				validateEffect(p_statuses, p_battleObjects, p_owner, effect);
			}
		}
	}

	namespace
	{
		void validateReward(
			const Registry<StatusDefinition> &p_statuses,
			const Registry<AbilityDefinition> &p_abilities,
			const std::string &p_owner,
			const FeatRewardSpec &p_reward)
		{
			const auto fail = [&p_owner, &p_reward](const std::string &p_message) {
				throw JsonError(
					p_reward.source.file,
					p_reward.source.jsonPath,
					"board '" + p_owner + "' reward '" + p_reward.id + "' " + p_message);
			};

			if (const auto *unlock = std::get_if<UnlockAbilityRewardSpec>(&p_reward.payload))
			{
				if (!p_abilities.contains(unlock->ability))
				{
					fail("unlocks unknown ability '" + unlock->ability + "'");
				}
			}
			else if (const auto *remove = std::get_if<RemoveAbilityRewardSpec>(&p_reward.payload))
			{
				if (!p_abilities.contains(remove->ability))
				{
					fail("removes unknown ability '" + remove->ability + "'");
				}
			}
			else if (const auto *passive = std::get_if<UnlockPassiveRewardSpec>(&p_reward.payload))
			{
				const StatusDefinition *status = p_statuses.tryGet(passive->status);
				if (status == nullptr)
				{
					fail("grants unknown passive status '" + passive->status + "'");
				}
				// A passive is an infinite status, and a permanently stunned creature never plays.
				if (isStunStatus(*status))
				{
					fail("grants the stun status '" + passive->status + "' as a permanent passive");
				}
			}

			// bonusStat references nothing, and changeForm is resolved in step 04 by the species
			// that selects the board.
		}
	}

	void validateCombatDefinitionGraph(
		const Registry<StatusDefinition> &p_statuses,
		const Registry<AbilityDefinition> &p_abilities,
		const Registry<BattleObjectDefinition> &p_battleObjects)
	{
		for (const auto &[id, status] : p_statuses)
		{
			for (const StatusHookSpec &hook : status.hooks)
			{
				validateEffects(p_statuses, p_battleObjects, id, hook.effects);
			}
		}

		for (const auto &[id, battleObject] : p_battleObjects)
		{
			for (const BattleObjectTriggerSpec &trigger : battleObject.triggers)
			{
				validateEffects(p_statuses, p_battleObjects, id, trigger.effects);
			}
		}

		for (const auto &[id, ability] : p_abilities)
		{
			validateEffects(p_statuses, p_battleObjects, id, ability.effects);
		}
	}

	void validateConditionReferences(
		const Registry<StatusDefinition> &p_statuses,
		const Registry<AbilityDefinition> &p_abilities,
		const std::vector<ConditionSpec> &p_conditions,
		const std::string &p_owner)
	{
		ConditionReferences references;
		collectConditionReferences(p_conditions, references);

		for (const ConditionReference &reference : references.abilities)
		{
			if (!p_abilities.contains(reference.id))
			{
				throw JsonError(
					reference.source.file,
					reference.source.jsonPath,
					"'" + p_owner + "' filters on unknown ability '" + reference.id + "'");
			}
		}
		for (const ConditionReference &reference : references.statuses)
		{
			if (!p_statuses.contains(reference.id))
			{
				throw JsonError(
					reference.source.file,
					reference.source.jsonPath,
					"'" + p_owner + "' filters on unknown status '" + reference.id + "'");
			}
		}

		// Tag filters resolve against no registry on purpose: a tag that matches nothing today is
		// legitimate, because later content may introduce it. Their grammar was checked at parse.
	}

	void validateFeatBoardGraph(
		const Registry<StatusDefinition> &p_statuses,
		const Registry<AbilityDefinition> &p_abilities,
		const Registry<FeatBoardDefinition> &p_featBoards)
	{
		for (const auto &[id, board] : p_featBoards)
		{
			for (const FeatNodeDefinition &node : board.nodes)
			{
				validateConditionReferences(
					p_statuses,
					p_abilities,
					node.requirements,
					"board '" + id + "' node '" + node.id + "'");

				for (const FeatRewardSpec &reward : node.rewards)
				{
					validateReward(p_statuses, p_abilities, id, reward);
				}
			}
		}
	}

	namespace
	{
		[[noreturn]] void failRule(
			const std::string &p_behaviourId,
			const AIRuleDefinition &p_rule,
			const std::string &p_message)
		{
			throw JsonError(
				p_rule.source.file,
				p_rule.source.jsonPath,
				"AI '" + p_behaviourId + "' rule '" + p_rule.id + "' " + p_message);
		}

		[[nodiscard]] std::set<std::string> collectStatusTags(const Registry<StatusDefinition> &p_statuses)
		{
			std::set<std::string> tags;
			for (const auto &[id, status] : p_statuses)
			{
				(void)id;
				tags.insert(status.tags.begin(), status.tags.end());
			}
			return tags;
		}

		void validateAICondition(
			const Registry<StatusDefinition> &p_statuses,
			const Registry<AbilityDefinition> &p_abilities,
			const std::set<std::string> &p_tags,
			const std::string &p_behaviourId,
			const AIRuleDefinition &p_rule,
			const AIConditionSpec &p_condition)
		{
			if (const auto *status = std::get_if<AIStatusCondition>(&p_condition))
			{
				if (const auto *byId = std::get_if<AIStatusIdReference>(&status->reference))
				{
					if (!p_statuses.contains(byId->statusId))
					{
						failRule(p_behaviourId, p_rule, "queries unknown status '" + byId->statusId + "'");
					}
				}
				else
				{
					const std::string &tag = std::get<AIStatusTagReference>(status->reference).tag;
					// An AI tag is a question about the content that exists now: a misspelled one is
					// simply false forever, which is exactly the bug that never gets reported.
					if (!p_tags.contains(tag))
					{
						failRule(
							p_behaviourId,
							p_rule,
							"queries status tag '" + tag + "', which no status definition carries");
					}
				}
			}
			else if (const auto *affordable = std::get_if<AIAbilityAffordableCondition>(&p_condition))
			{
				if (!p_abilities.contains(affordable->abilityId))
				{
					failRule(p_behaviourId, p_rule, "tests unknown ability '" + affordable->abilityId + "'");
				}
			}
		}
	}

	void validateAIGraph(
		const Registry<StatusDefinition> &p_statuses,
		const Registry<AbilityDefinition> &p_abilities,
		const Registry<AIBehaviourDefinition> &p_behaviours)
	{
		const std::set<std::string> tags = collectStatusTags(p_statuses);

		for (const auto &[id, behaviour] : p_behaviours)
		{
			for (const AIRuleDefinition &rule : behaviour.rules)
			{
				for (const AIConditionSpec &condition : rule.conditions)
				{
					validateAICondition(p_statuses, p_abilities, tags, id, rule, condition);
				}

				if (const auto *cast = std::get_if<AICastAbilityDecision>(&rule.decision))
				{
					if (!p_abilities.contains(cast->abilityId))
					{
						failRule(id, rule, "casts unknown ability '" + cast->abilityId + "'");
					}
				}
			}
		}
	}

	namespace
	{
		[[noreturn]] void failSpecies(const CreatureSpeciesDefinition &p_species, const std::string &p_message)
		{
			throw JsonError(
				p_species.source.file,
				p_species.source.jsonPath,
				"species '" + p_species.id + "' " + p_message);
		}

		[[nodiscard]] std::int64_t addChecked(
			const CreatureSpeciesDefinition &p_species,
			std::int64_t p_left,
			std::int64_t p_right)
		{
			constexpr std::int64_t Minimum = std::numeric_limits<std::int64_t>::min();
			constexpr std::int64_t Maximum = std::numeric_limits<std::int64_t>::max();

			if ((p_right > 0 && p_left > Maximum - p_right) || (p_right < 0 && p_left < Minimum - p_right))
			{
				failSpecies(p_species, "authors rewards whose sum overflows a 64-bit stat");
			}
			return p_left + p_right;
		}

		[[nodiscard]] std::int64_t rewardDelta(const BonusStatRewardSpec &p_bonus) noexcept
		{
			return p_bonus.stat == CreatureStat::Stamina ? p_bonus.staminaDelta.ticks() : p_bonus.amount;
		}

		// What one node grants for one stat: a node completes as a whole, so its rewards for that
		// stat sum into a single all-or-nothing delta.
		[[nodiscard]] std::int64_t nodeDelta(
			const CreatureSpeciesDefinition &p_species,
			const FeatNodeDefinition &p_node,
			CreatureStat p_stat)
		{
			std::int64_t delta = 0;
			for (const FeatRewardSpec &reward : p_node.rewards)
			{
				if (const auto *bonus = std::get_if<BonusStatRewardSpec>(&reward.payload); bonus != nullptr && bonus->stat == p_stat)
				{
					delta = addChecked(p_species, delta, rewardDelta(*bonus));
				}
			}
			return delta;
		}

		// The reachable range of one stat, without enumerating the 2^n completion sets: an ordinary
		// node contributes its delta or nothing, an exclusive group contributes its best (or worst)
		// member's delta or nothing, and the root always contributes - it starts completed.
		//
		// The range is a superset of what the gates actually allow, so it can only reject a board
		// that would need the gates to save it. That is the conservative side to be on.
		struct StatRange
		{
			std::int64_t minimum = 0;
			std::int64_t maximum = 0;
		};

		[[nodiscard]] StatRange reachableRange(
			const CreatureSpeciesDefinition &p_species,
			const FeatBoardDefinition &p_board,
			CreatureStat p_stat)
		{
			struct GroupRange
			{
				std::int64_t best = 0;
				std::int64_t worst = 0;
			};

			StatRange range;
			std::map<std::string, GroupRange> groups;

			for (const FeatNodeDefinition &node : p_board.nodes)
			{
				const std::int64_t delta = nodeDelta(p_species, node, p_stat);
				if (delta == 0)
				{
					continue;
				}

				if (node.kind == FeatNodeKind::Root)
				{
					// The root is completed the moment the creature exists, so its grant is not an
					// option the range gets to decline.
					range.minimum = addChecked(p_species, range.minimum, delta);
					range.maximum = addChecked(p_species, range.maximum, delta);
					continue;
				}

				if (node.exclusiveGroup.has_value())
				{
					GroupRange &group = groups[*node.exclusiveGroup];
					group.best = std::max(group.best, delta);
					group.worst = std::min(group.worst, delta);
					continue;
				}

				range.maximum = addChecked(p_species, range.maximum, std::max<std::int64_t>(0, delta));
				range.minimum = addChecked(p_species, range.minimum, std::min<std::int64_t>(0, delta));
			}

			for (const auto &[groupId, group] : groups)
			{
				(void)groupId;
				range.maximum = addChecked(p_species, range.maximum, group.best);
				range.minimum = addChecked(p_species, range.minimum, group.worst);
			}
			return range;
		}

		void validateRewardArithmetic(
			const CreatureSpeciesDefinition &p_species,
			const FeatBoardDefinition &p_board,
			const DerivationContext &p_context)
		{
			CreatureAttributes highest = p_species.baseAttributes;
			CreatureAttributes lowest = p_species.baseAttributes;

			for (const auto &[name, stat] : creatureStatNames())
			{
				(void)name;
				const StatRange range = reachableRange(p_species, p_board, stat);
				const BattleTime maximumStamina = BattleTime::fromTicks(range.maximum);
				const BattleTime minimumStamina = BattleTime::fromTicks(range.minimum);
				try
				{
					addToAttribute(highest, stat, range.maximum, maximumStamina);
					addToAttribute(lowest, stat, range.minimum, minimumStamina);
				} catch (const std::overflow_error &error)
				{
					failSpecies(p_species, std::string("on board '") + p_board.id + "': " + error.what());
				}
			}

			// Both extremes have to stay legal, because any completion set the board allows lands
			// between them.
			requireValidAttributes(
				highest,
				*p_context.gameRules,
				p_species.source,
				"species '" + p_species.id + "' fully completing board '" + p_board.id + "'");
			requireValidAttributes(
				lowest,
				*p_context.gameRules,
				p_species.source,
				"species '" + p_species.id + "' worst-casing board '" + p_board.id + "'");
		}

		// A creature has one loadout array, and there is no move-slot selection system to choose
		// what to drop. So the union of the defaults and every ability the board can unlock has to
		// fit, even though a removeAbility reward or an exclusive branch may make the real set
		// smaller. It never makes it larger.
		void validateAbilityCapacity(
			const CreatureSpeciesDefinition &p_species,
			const FeatBoardDefinition &p_board,
			const DerivationContext &p_context)
		{
			std::set<std::string> reachable(p_species.defaultAbilityIds.begin(), p_species.defaultAbilityIds.end());
			for (const FeatNodeDefinition &node : p_board.nodes)
			{
				for (const FeatRewardSpec &reward : node.rewards)
				{
					if (const auto *unlock = std::get_if<UnlockAbilityRewardSpec>(&reward.payload))
					{
						reachable.insert(unlock->ability);
					}
				}
			}

			if (reachable.size() > p_context.gameRules->battle.abilitySlotCapacity)
			{
				failSpecies(
					p_species,
					"can reach " + std::to_string(reachable.size()) + " abilities through board '" + p_board.id +
						"', over the " + std::to_string(p_context.gameRules->battle.abilitySlotCapacity) +
						" loadout slots");
			}
		}

		// A Feat Board climbs the evolution chain; it never walks back down it. Two rules say so:
		// a node that gates on a form and changes it may only change it upward, and the changeForm
		// rewards, read in board definition order, name forms of non-decreasing tier.
		void validateFormTiers(const CreatureSpeciesDefinition &p_species, const FeatBoardDefinition &p_board)
		{
			const auto tierOf = [&p_species](const std::string &p_formId) {
				const CreatureFormDefinition *form = p_species.tryForm(p_formId);
				return form == nullptr ? 0U : form->tier;
			};

			std::uint32_t previous = 0;
			bool seen = false;
			for (const FeatNodeDefinition &node : p_board.nodes)
			{
				for (const FeatRewardSpec &reward : node.rewards)
				{
					const auto *changeForm = std::get_if<ChangeFormRewardSpec>(&reward.payload);
					if (changeForm == nullptr)
					{
						continue;
					}

					const std::uint32_t target = tierOf(changeForm->form);
					if (node.fromForm.has_value() && target < tierOf(*node.fromForm))
					{
						throw JsonError(
							reward.source.file,
							reward.source.jsonPath,
							"species '" + p_species.id + "': node '" + node.id + "' evolves from '" + *node.fromForm +
								"' into the lower-tier form '" + changeForm->form + "'");
					}
					if (seen && target < previous)
					{
						throw JsonError(
							reward.source.file,
							reward.source.jsonPath,
							"species '" + p_species.id + "': board '" + p_board.id +
								"' authors its changeForm rewards out of tier order; '" + changeForm->form +
								"' is tier " + std::to_string(target) + " after a tier-" + std::to_string(previous) +
								" form");
					}
					previous = target;
					seen = true;
				}
			}
		}
	}

	void validateSpeciesGraph(const DerivationContext &p_context)
	{
		if (p_context.gameRules == nullptr || p_context.statuses == nullptr || p_context.abilities == nullptr ||
			p_context.featBoards == nullptr || p_context.species == nullptr)
		{
			throw std::invalid_argument("derivation context is missing a registry");
		}

		for (const auto &[id, species] : *p_context.species)
		{
			for (const std::string &abilityId : species.defaultAbilityIds)
			{
				if (!p_context.abilities->contains(abilityId))
				{
					failSpecies(species, "knows unknown ability '" + abilityId + "'");
				}
			}
			for (const std::string &statusId : species.defaultPassiveStatusIds)
			{
				const StatusDefinition *status = p_context.statuses->tryGet(statusId);
				if (status == nullptr)
				{
					failSpecies(species, "carries unknown passive status '" + statusId + "'");
				}
				// A passive is an infinite status, and a permanently stunned creature never plays.
				if (isStunStatus(*status))
				{
					failSpecies(species, "carries the stun status '" + statusId + "' as a permanent passive");
				}
			}

			const FeatBoardDefinition *board = p_context.featBoards->tryGet(species.featBoardId);
			if (board == nullptr)
			{
				failSpecies(species, "selects unknown feat board '" + species.featBoardId + "'");
			}

			// The other half of the step-03 form contract: the board kept the file and JSON path of
			// every fromForm gate and changeForm reward exactly so this failure points at the board
			// line the author has to fix, and names the species that made it wrong.
			std::set<std::string, std::less<>> formIds;
			for (const CreatureFormDefinition &form : species.forms)
			{
				formIds.insert(form.id);
			}
			validateFeatBoardFormReferences(*board, formIds, id);
			validateFormTiers(species, *board);
			validateAbilityCapacity(species, *board, p_context);
			validateRewardArithmetic(species, *board, p_context);

			if (species.tamingProfile.has_value())
			{
				validateConditionReferences(
					*p_context.statuses,
					*p_context.abilities,
					species.tamingProfile->conditions,
					"species '" + id + "' taming profile");
			}

			// And finally the baseline itself: a fresh creature of this species has to be a legal
			// creature, root rewards included.
			(void)deriveCreatureState(species, *board, makeFreshFeatBoardProgress(*board), p_context);
		}
	}

	namespace
	{
		[[noreturn]] void failSpawn(
			const std::string &p_encounterId,
			const EncounterSpawnDefinition &p_spawn,
			const std::string &p_message)
		{
			throw JsonError(
				p_spawn.source.file,
				p_spawn.source.jsonPath,
				"encounter '" + p_encounterId + "' member '" + p_spawn.id + "' " + p_message);
		}

		void validateSpawn(
			const std::string &p_encounterId,
			const EncounterSpawnDefinition &p_spawn,
			const Registry<AIBehaviourDefinition> &p_behaviours,
			const DerivationContext &p_context)
		{
			if (!p_context.species->contains(p_spawn.speciesId))
			{
				failSpawn(p_encounterId, p_spawn, "is of unknown species '" + p_spawn.speciesId + "'");
			}

			const AIBehaviourDefinition *behaviour = p_behaviours.tryGet(p_spawn.aiBehaviourId);
			if (behaviour == nullptr)
			{
				failSpawn(p_encounterId, p_spawn, "uses unknown AI behaviour '" + p_spawn.aiBehaviourId + "'");
			}

			// Built exactly as step 06 will build it - through the one factory - so an illegal
			// completed-node preset fails at load rather than at battle entry.
			CreatureUnit unit;
			try
			{
				unit = makeCreatureUnit(
					CreatureInstanceId::fromSerial(1),
					p_spawn.speciesId,
					p_spawn.completedFeatNodeIds,
					p_context);
			} catch (const JsonError &)
			{
				throw;
			} catch (const std::exception &error)
			{
				failSpawn(p_encounterId, p_spawn, std::string("is not a legal creature: ") + error.what());
			}

			// An enemy may not be authored to cast a move it does not know: that rule would simply
			// never fire, and a silently passive enemy is a bug nobody reports.
			for (const std::string &abilityId : aiCastAbilityReferences(*behaviour))
			{
				if (std::ranges::find(unit.derived.abilityIds, abilityId) == unit.derived.abilityIds.end())
				{
					failSpawn(
						p_encounterId,
						p_spawn,
						"is driven by AI '" + p_spawn.aiBehaviourId + "', which casts '" + abilityId +
							"', an ability its derived loadout does not hold");
				}
			}
		}
	}

	void validateEncounterGraph(
		const Registry<EncounterDefinition> &p_encounters,
		const Registry<AIBehaviourDefinition> &p_behaviours,
		const DerivationContext &p_context)
	{
		if (p_context.species == nullptr)
		{
			throw std::invalid_argument("derivation context is missing a registry");
		}

		for (const auto &[encounterId, encounter] : p_encounters)
		{
			for (const EncounterTierDefinition &tier : encounter.tiers)
			{
				for (const EncounterTeamDefinition &team : tier.teams)
				{
					for (const EncounterSpawnDefinition &spawn : team.members)
					{
						validateSpawn(encounterId, spawn, p_behaviours, p_context);
					}
				}
			}
		}
	}

	void validateBiomeEncounterLinks(
		const Registry<BiomeDefinition> &p_biomes,
		const Registry<EncounterDefinition> &p_encounters)
	{
		for (const auto &[id, biome] : p_biomes)
		{
			const bool generated = biome.worldgen.has_value();
			if (!generated)
			{
				// An interior-only biome (a cave) has no Bush to step into yet. Step 19 revisits it
				// when the interior encounter policy exists.
				continue;
			}

			// The parser already refused a generated biome with no link, so this is the reference
			// half of the contract rather than the presence half.
			const std::string &tableId = biome.wildEncounterTableId.value();
			const EncounterDefinition *encounter = p_encounters.tryGet(tableId);
			if (encounter == nullptr)
			{
				throw JsonError(
					biome.source.file,
					biome.source.jsonPath + ".wildEncounterTable",
					"biome '" + id + "' references unknown encounter table '" + tableId + "'");
			}
			if (encounter->kind != EncounterKind::Wild || !encounter->repeatable)
			{
				throw JsonError(
					biome.source.file,
					biome.source.jsonPath + ".wildEncounterTable",
					"biome '" + id + "' references '" + tableId +
						"', which is not a repeatable wild encounter; a Bush leads to wild creatures");
			}
		}
	}
}
