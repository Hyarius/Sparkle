#include "battle/presentation/battle_hud_view_model_builder.hpp"

#include "battle/presentation/ability_summary_formatter.hpp"
#include "core/registries.hpp"

#include <algorithm>
#include <array>
#include <limits>
#include <type_traits>

namespace
{
	[[nodiscard]] bool isCombatant(const pg::BattleUnitSnapshot &p_unit) noexcept
	{
		return p_unit.placed && p_unit.cell.has_value() && p_unit.removalReason == pg::RemovalReason::None && p_unit.health > 0;
	}

	[[nodiscard]] std::int64_t shieldValue(const pg::BattleUnitSnapshot &p_unit) noexcept
	{
		std::int64_t result = 0;
		for (const pg::BattleShieldSnapshot &shield : p_unit.shields)
		{
			result += shield.remainingAmount;
		}
		return result;
	}

	[[nodiscard]] std::string durationText(const pg::DurationSnapshot &p_duration)
	{
		return std::visit([](const auto &duration) -> std::string {
			using Duration = std::decay_t<decltype(duration)>;
			if constexpr (std::is_same_v<Duration, pg::TimelineDurationSnapshot>)
			{
				return std::to_string(duration.expiresAt.ticks()) + " ms";
			}
			else if constexpr (std::is_same_v<Duration, pg::OwnerActivationDurationSnapshot>)
			{
				return std::to_string(duration.remainingActivations) + " turns";
			}
			else
			{
				return "permanent";
			}
		},
						  p_duration);
	}
}

namespace pg
{
	std::vector<TurnForecastEntry> forecastActivations(const BattleSnapshot &p_snapshot, std::size_t p_maximumEntries)
	{
		struct ProjectedUnit
		{
			const BattleUnitSnapshot *unit = nullptr;
			BattleTime fill{};
		};
		std::vector<ProjectedUnit> units;
		for (const BattleUnitSnapshot &unit : p_snapshot.units)
		{
			if (!isCombatant(unit) || unit.stamina.isZero())
			{
				continue;
			}
			units.push_back({.unit = &unit, .fill = unit.turnBarFill});
		}
		if (p_snapshot.activeUnit.has_value())
		{
			for (ProjectedUnit &unit : units)
			{
				if (unit.unit->id == *p_snapshot.activeUnit)
				{
					unit.fill = BattleTime{};
				}
			}
		}
		std::vector<TurnForecastEntry> result;
		BattleTime now = p_snapshot.elapsed;
		while (!units.empty() && result.size() < p_maximumEntries)
		{
			bool anyReady = std::any_of(units.begin(), units.end(), [](const ProjectedUnit &unit) {
				return unit.fill >= unit.unit->stamina;
			});
			if (!anyReady)
			{
				std::optional<BattleTime> delta;
				for (const ProjectedUnit &unit : units)
				{
					const BattleTime remaining = unit.unit->stamina - unit.fill;
					if (!delta.has_value() || remaining < *delta)
					{
						delta = remaining;
					}
				}
				if (!delta.has_value() || delta->isZero())
				{
					break;
				}
				now = now + *delta;
				for (ProjectedUnit &unit : units)
				{
					unit.fill = std::min(unit.unit->stamina, unit.fill + *delta);
				}
			}
			auto winner = units.end();
			for (auto iterator = units.begin(); iterator != units.end(); ++iterator)
			{
				if (iterator->fill < iterator->unit->stamina)
				{
					continue;
				}
				const auto order = [](const ProjectedUnit &value) {
					return std::tuple{value.unit->side == BattleSide::Player ? 0 : 1, value.unit->rosterOrder, value.unit->id.value()};
				};
				if (winner == units.end() || order(*iterator) < order(*winner))
				{
					winner = iterator;
				}
			}
			if (winner == units.end())
			{
				break;
			}
			result.push_back({.unit = winner->unit->id, .readyAt = now});
			winner->fill = BattleTime{};
		}
		return result;
	}

	BattleHudViewModel BattleHudViewModelBuilder::build(
		const BattleSnapshot &p_snapshot,
		const Registries &p_registries,
		const BattleInteractionState &p_interaction)
	{
		BattleHudViewModel model;
		model.phase = p_snapshot.phase;
		model.playerTeam.assign(6, CreatureCardViewModel{.side = BattleSide::Player});
		model.opponentTeam.assign(6, CreatureCardViewModel{.side = BattleSide::Enemy});
		const auto selectedPlacement = std::get_if<PlacementSelecting>(&p_interaction);
		const auto makeStatus = [&p_registries](const BattleStatusSnapshot &p_status) {
			StatusViewModel result{
				.id = p_status.definitionId,
				.stacks = p_status.stacks,
				.duration = durationText(p_status.duration)};
			if (const StatusDefinition *definition = p_registries.statuses().tryGet(p_status.definitionId); definition != nullptr)
			{
				result.displayName = definition->displayNameKey;
				result.description = definition->descriptionKey;
				result.icon = {static_cast<std::uint32_t>(definition->icon.x), static_cast<std::uint32_t>(definition->icon.y)};
			}
			return result;
		};

		const auto makeCard = [&](const BattleUnitSnapshot &unit) {
			CreatureCardViewModel card;
			card.unit = unit.id;
			card.creature = unit.persistentCreatureId;
			card.side = unit.side;
			card.displayName = unit.speciesId;
			if (const CreatureSpeciesDefinition *species = p_registries.species().tryGet(unit.speciesId); species != nullptr)
			{
				card.displayName = species->displayNameKey;
				if (const CreatureFormDefinition *form = species->tryForm(unit.formId); form != nullptr)
				{
					card.displayName = form->displayNameKey;
					card.icon = {static_cast<std::uint32_t>(form->icon.x), static_cast<std::uint32_t>(form->icon.y)};
					card.tint = form->presentation.tint;
				}
			}
			card.health = {.current = unit.health, .maximum = unit.maxHealth, .label = "HP"};
			card.shield = shieldValue(unit);
			card.details.health = card.health;
			card.details.actionPoints = {.current = unit.actionPoints, .maximum = unit.maxActionPoints, .label = "AP"};
			card.details.movementPoints = {.current = unit.movementPoints, .maximum = unit.maxMovementPoints, .label = "MP"};
			card.details.strength = unit.effectiveAttributes.strength;
			card.details.armor = unit.effectiveAttributes.armor;
			card.details.magicPower = unit.effectiveAttributes.magicPower;
			card.details.resistance = unit.effectiveAttributes.resistance;
			card.details.shield = card.shield;
			card.details.range = unit.range;
			card.details.stamina = unit.stamina;
			for (const std::string &abilityId : unit.abilityIds)
			{
				const AbilityDefinition *ability = p_registries.abilities().tryGet(abilityId);
				card.details.abilities.push_back(ability == nullptr ? abilityId : ability->displayNameKey);
			}
			for (const BattleStatusSnapshot &status : unit.passiveStatuses)
			{
				card.details.passiveEffects.push_back(makeStatus(status));
			}
			for (const BattleStatusSnapshot &status : unit.transientStatuses)
			{
				card.details.activeEffects.push_back(makeStatus(status));
			}
			card.readyAt = p_snapshot.elapsed + (unit.stamina - unit.turnBarFill);
			card.readyIn = unit.stamina > unit.turnBarFill ? unit.stamina - unit.turnBarFill : BattleTime{};
			card.placed = unit.placed;
			card.defeated = unit.removalReason != RemovalReason::None || unit.health <= 0;
			card.active = p_snapshot.activeUnit == unit.id;
			card.selectedForPlacement = selectedPlacement != nullptr && selectedPlacement->selected == unit.persistentCreatureId;
			return card;
		};

		for (const BattleUnitSnapshot &unit : p_snapshot.units)
		{
			if (unit.rosterOrder >= 6)
			{
				continue;
			}
			(unit.side == BattleSide::Player ? model.playerTeam : model.opponentTeam)[unit.rosterOrder] = makeCard(unit);
		}

		for (const TurnForecastEntry &entry : forecastActivations(p_snapshot, 10))
		{
			const auto unit = std::find_if(p_snapshot.units.begin(), p_snapshot.units.end(), [&entry](const BattleUnitSnapshot &candidate) {
				return candidate.id == entry.unit;
			});
			if (unit == p_snapshot.units.end())
			{
				continue;
			}
			model.forecast.push_back({.unit = entry.unit, .shortName = unit->speciesId, .side = unit->side, .readyAt = entry.readyAt, .offsetFromNow = entry.readyAt - p_snapshot.elapsed});
		}

		const BattleUnitSnapshot *active = nullptr;
		if (p_snapshot.phase == BattlePhase::Activation && p_snapshot.activeUnit.has_value())
		{
			auto iterator = std::find_if(p_snapshot.units.begin(), p_snapshot.units.end(), [&p_snapshot](const BattleUnitSnapshot &unit) {
				return unit.id == *p_snapshot.activeUnit;
			});
			if (iterator != p_snapshot.units.end())
			{
				active = &*iterator;
			}
		}
		if (active != nullptr)
		{
			ActiveUnitViewModel value;
			value.unit = active->id;
			value.displayName = active->speciesId;
			value.side = active->side;
			value.health = {.current = active->health, .maximum = active->maxHealth, .label = "HP"};
			value.actionPoints = {.current = active->actionPoints, .maximum = active->maxActionPoints, .label = "AP"};
			value.movementPoints = {.current = active->movementPoints, .maximum = active->maxMovementPoints, .label = "MP"};
			value.shield = shieldValue(*active);
			value.stamina = active->stamina;
			for (const BattleStatusSnapshot &status : active->passiveStatuses)
			{
				value.statuses.push_back(makeStatus(status));
			}
			for (const BattleStatusSnapshot &status : active->transientStatuses)
			{
				value.statuses.push_back(makeStatus(status));
			}
			model.activeUnit = std::move(value);
			for (std::size_t index = 0; index < model.abilities.size(); ++index)
			{
				AbilitySlotViewModel &slot = model.abilities[index];
				slot.shortcut = index + 1;
				if (index >= active->abilityIds.size())
				{
					continue;
				}
				slot.abilityId = active->abilityIds[index];
				if (const AbilityDefinition *ability = p_registries.abilities().tryGet(*slot.abilityId); ability != nullptr)
				{
					slot.name = ability->displayNameKey;
					slot.description = ability->descriptionKey;
					slot.costText = formatAbilityCost(*ability);
					slot.rangeText = formatAbilityRange(*ability);
					slot.icon = {static_cast<std::uint32_t>(ability->icon.x), static_cast<std::uint32_t>(ability->icon.y)};
					for (const EffectApplication &effect : ability->effects)
					{
						slot.effects.push_back(effect.id);
					}
				}
				slot.selected = std::holds_alternative<SelectingAbility>(p_interaction) && std::get<SelectingAbility>(p_interaction).abilityId == *slot.abilityId;
				slot.enabled = active->side == BattleSide::Player && std::holds_alternative<AwaitingAction>(p_interaction);
				if (!slot.enabled)
				{
					slot.disabledReason = CommandRejection::CommandUnavailable;
				}
			}
		}

		const BattleUnitSnapshot *actionBarUnit = active;
		if (p_snapshot.phase == BattlePhase::Deployment && selectedPlacement != nullptr)
		{
			auto selected = std::find_if(p_snapshot.units.begin(), p_snapshot.units.end(), [selectedPlacement](const BattleUnitSnapshot &unit) {
				return unit.side == BattleSide::Player && unit.persistentCreatureId == selectedPlacement->selected;
			});
			if (selected != p_snapshot.units.end())
			{
				actionBarUnit = &*selected;
			}
		}
		if (actionBarUnit != nullptr)
		{
			ActionBarViewModel actionBar;
			actionBar.displayName = actionBarUnit->speciesId;
			actionBar.side = actionBarUnit->side;
			actionBar.health = {.current = actionBarUnit->health, .maximum = actionBarUnit->maxHealth, .label = "HP"};
			actionBar.actionPoints = {.current = actionBarUnit->actionPoints, .maximum = actionBarUnit->maxActionPoints, .label = "AP"};
			actionBar.movementPoints = {.current = actionBarUnit->movementPoints, .maximum = actionBarUnit->maxMovementPoints, .label = "MP"};
			for (const BattleStatusSnapshot &status : actionBarUnit->passiveStatuses)
			{
				actionBar.effects.push_back(makeStatus(status));
			}
			for (const BattleStatusSnapshot &status : actionBarUnit->transientStatuses)
			{
				actionBar.effects.push_back(makeStatus(status));
			}
			model.actionBar = std::move(actionBar);
		}

		model.deployment.instruction = "Select a creature, then click a highlighted support cell.";
		for (const BattleUnitSnapshot &unit : p_snapshot.units)
		{
			if (unit.side == BattleSide::Player && unit.removalReason == RemovalReason::None)
			{
				++model.deployment.requiredCount;
				if (unit.placed)
				{
					++model.deployment.placedCount;
				}
			}
		}
		model.deployment.readyEnabled = p_snapshot.phase == BattlePhase::Deployment && model.deployment.placedCount == model.deployment.requiredCount && model.deployment.requiredCount > 0;
		if (!model.deployment.readyEnabled)
		{
			model.deployment.readyDisabledReason = CommandRejection::DeploymentIncomplete;
		}
		if (p_snapshot.phase == BattlePhase::Deployment)
		{
			model.statusLine = "Deployment";
		}
		else if (p_snapshot.phase == BattlePhase::Terminal)
		{
			model.statusLine = p_snapshot.outcome == BattleOutcome::Aborted ? "Technical error" : std::string(toString(p_snapshot.outcome));
		}
		else if (active != nullptr && active->side == BattleSide::Enemy)
		{
			model.statusLine = "Enemy acting...";
		}
		else
		{
			model.statusLine = "Your turn";
		}
		return model;
	}

	BattleHudViewModel BattleHudViewModelBuilder::build(
		const BattleSnapshot &p_snapshot,
		const Registries &p_registries,
		const BattleInteractionController &p_interaction)
	{
		BattleHudViewModel model = build(p_snapshot, p_registries, p_interaction.state());
		for (std::size_t index = 0; index < model.abilities.size(); ++index)
		{
			AbilitySlotViewModel &slot = model.abilities[index];
			if (!slot.abilityId.has_value())
			{
				continue;
			}
			const AbilitySlotAvailability availability = p_interaction.abilityAvailability(index);
			slot.enabled = availability.enabled;
			slot.disabledReason = availability.rejection;
		}
		const AbilitySlotAvailability ready = p_interaction.deploymentReadyAvailability();
		model.deployment.readyEnabled = ready.enabled;
		model.deployment.readyDisabledReason = ready.rejection;
		return model;
	}
}
