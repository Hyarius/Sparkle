#pragma once

#include "battle/battle_lifecycle.hpp"

#include "structures/math/spk_vector3.hpp"

#include <cstdint>
#include <optional>
#include <string>
#include <unordered_map>
#include <variant>

namespace pg
{
	class Registries;
	class VoxelWorld;
	struct WorldPlan;
	struct PlayerData;

	struct TeleportRequest
	{
		spk::Vector3Int destination{};
	};

	// Scripted content has no runtime producer yet, but keeping the request slot makes the portal
	// > scripted > Bush ordering explicit instead of relying on subscription order.
	struct ScriptedEncounterRequest
	{
		std::string encounterDefinitionId;
	};

	using ExplorationInteraction = std::variant<std::monostate, TeleportRequest, BattleStartRequest, ScriptedEncounterRequest>;

	struct ExplorationInteractionResolution
	{
		ExplorationInteraction interaction;
		// A Bush consumes one ordinal even when its authored chance rejects.  Named requests consume
		// only when they were valid and fully resolved.
		bool consumesEncounterOrdinal = false;
	};

	// Read-only arrival resolver.  It never queues a mode transition or mutates PlayerData; the
	// widget publishes the requested serial only after receiving this value.
	class ExplorationInteractionResolver
	{
	private:
		const Registries &_registries;
		const VoxelWorld &_world;
		const WorldPlan &_plan;
		const std::unordered_map<spk::Vector3Int, spk::Vector3Int> &_portalTargets;
		const PlayerData &_player;
		std::uint64_t _worldSeed = 0;

		[[nodiscard]] std::optional<const EncounterDefinition *> _wildEncounterAt(
			const spk::Vector3Int &p_supportCell) const;
		[[nodiscard]] bool _alreadyCleared(const EncounterDefinition &p_definition) const;
		[[nodiscard]] BattleStartRequest _resolve(
			const EncounterDefinition &p_definition,
			const spk::Vector3Int &p_supportCell,
			VoxelOrientation p_approach,
			std::string p_instanceId,
			std::uint64_t p_ordinal,
			bool p_rollTrigger) const;

	public:
		ExplorationInteractionResolver(
			const Registries &p_registries,
			const VoxelWorld &p_world,
			const WorldPlan &p_plan,
			const std::unordered_map<spk::Vector3Int, spk::Vector3Int> &p_portalTargets,
			const PlayerData &p_player,
			std::uint64_t p_worldSeed);

		[[nodiscard]] ExplorationInteractionResolution resolvePlayerArrival(
			const spk::Vector3Int &p_supportCell,
			VoxelOrientation p_approach,
			std::uint64_t p_arrivalSequence) const;
		[[nodiscard]] ExplorationInteractionResolution resolveNamedEncounter(
			std::string_view p_encounterId,
			const spk::Vector3Int &p_returnCell,
			VoxelOrientation p_approach,
			std::uint64_t p_encounterOrdinal,
			std::optional<std::string> p_debugAutoplayProfileId = std::nullopt) const;
	};
}
