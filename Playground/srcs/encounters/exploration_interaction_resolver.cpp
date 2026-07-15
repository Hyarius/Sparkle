#include "encounters/exploration_interaction_resolver.hpp"

#include "battle/battle_rng.hpp"
#include "core/deterministic_random.hpp"
#include "core/registries.hpp"
#include "player/player_data.hpp"
#include "world/biome_definition.hpp"
#include "world/generator/world_plan.hpp"
#include "world/voxel_world.hpp"

#include <limits>
#include <stdexcept>

namespace
{
	[[nodiscard]] std::string wildIdentity(
		std::string_view p_tableId,
		const spk::Vector3Int &p_cell,
		std::uint64_t p_ordinal)
	{
		return "battle/wild/" + std::string(p_tableId) + "/" + std::to_string(p_cell.x) + "/" +
			   std::to_string(p_cell.y) + "/" + std::to_string(p_cell.z) + "/" + std::to_string(p_ordinal);
	}
}

namespace pg
{
	ExplorationInteractionResolver::ExplorationInteractionResolver(
		const Registries &p_registries,
		const VoxelWorld &p_world,
		const WorldPlan &p_plan,
		const std::unordered_map<spk::Vector3Int, spk::Vector3Int> &p_portalTargets,
		const PlayerData &p_player,
		std::uint64_t p_worldSeed) :
		_registries(p_registries),
		_world(p_world),
		_plan(p_plan),
		_portalTargets(p_portalTargets),
		_player(p_player),
		_worldSeed(p_worldSeed)
	{
	}

	std::optional<const EncounterDefinition *> ExplorationInteractionResolver::_wildEncounterAt(
		const spk::Vector3Int &p_supportCell) const
	{
		const spk::Vector3Int bushCell = p_supportCell + spk::Vector3Int{0, 1, 0};
		const spk::VoxelCell *voxel = _world.tryCell(bushCell);
		const VoxelDefinition *voxelDefinition = voxel == nullptr ? nullptr : _world.tryDefinition(*voxel);
		if (voxelDefinition == nullptr || !voxelDefinition->data.hasTag("Bush"))
		{
			return std::nullopt;
		}

		const int column = _plan.cellIndexFromWorld(p_supportCell.x);
		const int row = _plan.cellIndexFromWorld(p_supportCell.z);
		if (!_plan.zone.contains(row, column))
		{
			return std::nullopt;
		}
		const int zoneIndex = _plan.zone.at(row, column);
		if (zoneIndex < 0 || static_cast<std::size_t>(zoneIndex) >= _plan.zones.size())
		{
			return std::nullopt;
		}
		const PlanZone &zone = _plan.zones[static_cast<std::size_t>(zoneIndex)];
		if (zone.biomeIndex >= _plan.biomes.size())
		{
			return std::nullopt;
		}
		const BiomeDefinition &biome = _registries.biomes().get(_plan.biomes[zone.biomeIndex].id);
		if (!biome.wildEncounterTableId.has_value())
		{
			return std::nullopt;
		}
		const EncounterDefinition *encounter = _registries.encounters().tryGet(*biome.wildEncounterTableId);
		return encounter == nullptr ? std::nullopt : std::optional<const EncounterDefinition *>(encounter);
	}

	bool ExplorationInteractionResolver::_alreadyCleared(const EncounterDefinition &p_definition) const
	{
		switch (p_definition.kind)
		{
		case EncounterKind::Trainer:
			return _player.clearedTrainerIds.contains(p_definition.id);
		case EncounterKind::Gym:
			return _player.clearedGymIds.contains(p_definition.id);
		case EncounterKind::Special:
			return _player.clearedSpecialEncounterIds.contains(p_definition.id);
		case EncounterKind::Wild:
		case EncounterKind::Debug:
			return false;
		}
		return false;
	}

	BattleStartRequest ExplorationInteractionResolver::_resolve(
		const EncounterDefinition &p_definition,
		const spk::Vector3Int &p_supportCell,
		VoxelOrientation p_approach,
		std::string p_instanceId,
		std::uint64_t p_ordinal,
		bool p_rollTrigger) const
	{
		const std::uint64_t resolutionSeed =
			deterministic::deriveSeed(_worldSeed, p_instanceId + "/encounter-resolution");
		const std::uint64_t combatSeed = deterministic::deriveSeed(_worldSeed, p_instanceId + "/combat");
		BattleRng rng(resolutionSeed);
		if (p_rollTrigger && !rollEncounterTrigger(p_definition, rng))
		{
			throw std::runtime_error("encounter trigger rejected");
		}
		return BattleStartRequest{
			.encounter = resolveEncounter(p_definition, _player.encounterTier(), rng),
			.worldCell = p_supportCell,
			.playerApproach = p_approach,
			.stableEncounterInstanceId = std::move(p_instanceId),
			.encounterOrdinal = p_ordinal,
			.encounterResolutionSeed = resolutionSeed,
			.combatSeed = combatSeed};
	}

	ExplorationInteractionResolution ExplorationInteractionResolver::resolvePlayerArrival(
		const spk::Vector3Int &p_supportCell,
		VoxelOrientation p_approach,
		std::uint64_t p_arrivalSequence) const
	{
		if (const auto portal = _portalTargets.find(p_supportCell); portal != _portalTargets.end())
		{
			return {.interaction = TeleportRequest{portal->second}};
		}
		const std::optional<const EncounterDefinition *> wild = _wildEncounterAt(p_supportCell);
		if (!wild.has_value())
		{
			return {};
		}
		// A Bush arrival is an encounter attempt whether the chance wins or loses.  Overflow leaves
		// the run unchanged rather than wrapping an identity back to zero.
		if (p_arrivalSequence == std::numeric_limits<std::uint64_t>::max())
		{
			return {};
		}
		try
		{
			return {
				.interaction = _resolve(
					**wild,
					p_supportCell,
					p_approach,
					wildIdentity((**wild).id, p_supportCell, p_arrivalSequence),
					p_arrivalSequence,
					true),
				.consumesEncounterOrdinal = true};
		} catch (const std::runtime_error &error)
		{
			if (std::string_view(error.what()) == "encounter trigger rejected")
			{
				return {.consumesEncounterOrdinal = true};
			}
			throw;
		}
	}

	ExplorationInteractionResolution ExplorationInteractionResolver::resolveNamedEncounter(
		std::string_view p_encounterId,
		const spk::Vector3Int &p_returnCell,
		VoxelOrientation p_approach,
		std::uint64_t p_encounterOrdinal,
		std::optional<std::string> p_debugAutoplayProfileId) const
	{
		if (p_encounterOrdinal == std::numeric_limits<std::uint64_t>::max())
		{
			return {};
		}
		const EncounterDefinition *definition = _registries.encounters().tryGet(std::string(p_encounterId));
		if (definition == nullptr || _alreadyCleared(*definition))
		{
			return {};
		}
		if (p_debugAutoplayProfileId.has_value() &&
			(!_registries.aiBehaviours().contains(*p_debugAutoplayProfileId) || p_debugAutoplayProfileId->empty()))
		{
			return {};
		}
		BattleStartRequest request = _resolve(
			*definition,
			p_returnCell,
			p_approach,
			"battle/debug/" + definition->id + "/" + std::to_string(p_encounterOrdinal),
			p_encounterOrdinal,
			false);
		request.debugAutoplayPlayerProfileId = std::move(p_debugAutoplayProfileId);
		return {.interaction = std::move(request), .consumesEncounterOrdinal = true};
	}
}
