#include "encounters/encounter_emitter.hpp"

#include "core/game_context.hpp"
#include "encounters/biome.hpp"
#include "voxel/voxel_definition.hpp"
#include "world/voxel_world.hpp"

#include <iostream>
#include <array>
#include <algorithm>

namespace pg
{
	EncounterEmitter::EncounterEmitter(GameContext &p_context, spk::Vector2Int p_boardSize) :
		_context(p_context),
		_resolver(p_context.world.seed),
		_boardSize(p_boardSize),
		_moveContract(_context.events.playerMoved.subscribe([this](spk::Vector3Int p_cell) {
			_onPlayerMoved(p_cell);
		}))
	{
	}

	void EncounterEmitter::_onPlayerMoved(const spk::Vector3Int &p_standableCell)
	{
		if (!_enabled || !_context.world.explorationActive || _context.world.world == nullptr ||
			_context.world.activeBiome == nullptr)
		{
			return;
		}
		if (std::chrono::steady_clock::now() < _frozenUntil)
		{
			_resolver.observeCell(p_standableCell);
			return;
		}

		for (const BiomeEncounterRule &rule : _context.world.activeBiome->encounterRules)
		{
			const std::array cells = {
				p_standableCell,
				p_standableCell + spk::Vector3Int{0, 1, 0}};
			const bool matches = std::ranges::any_of(cells, [&](const spk::Vector3Int &p_cell) {
				const VoxelCell *voxel = _context.world.world->tryCell(p_cell);
				const VoxelDefinition *definition = voxel == nullptr ? nullptr : _context.world.world->tryDefinition(*voxel);
				return definition != nullptr && definition->data.hasTag(rule.trigger);
			});
			if (!matches)
			{
				continue;
			}
			const std::optional<ResolvedEncounter> resolved =
				_resolver.tryRoll(rule, p_standableCell, static_cast<int>(_context.clearedGyms.size()));
			if (!resolved.has_value())
			{
				return;
			}

			EncounterSpawn spawn{
				.displayName = resolved->displayName,
				.enemyTeam = resolved->team,
				.allowsTaming = true,
				.boardSize = resolved->boardSize.value_or(_boardSize),
				.placementStyle = PlacementStyle::Random};
			_frozenUntil = std::chrono::steady_clock::now() + std::chrono::seconds(2);
			std::cout << encounterSummary(spawn) << std::endl;
			_context.events.encounterTriggered.trigger(spawn);
			return;
		}
		_resolver.observeCell(p_standableCell);
	}

	void EncounterEmitter::setEnabled(bool p_enabled) noexcept
	{
		_enabled = p_enabled;
	}

	void EncounterEmitter::suppressCell(const spk::Vector3Int &p_cell) noexcept
	{
		_resolver.observeCell(p_cell);
		_frozenUntil = std::chrono::steady_clock::now() + std::chrono::seconds(2);
	}

	bool EncounterEmitter::enabled() const noexcept
	{
		return _enabled;
	}

	std::string encounterSummary(const EncounterSpawn &p_spawn)
	{
		const std::string species = p_spawn.enemyTeam.empty() ? "?" : p_spawn.enemyTeam.front().speciesId;
		return "Encounter: " + p_spawn.displayName + " (" + species + ") \xE2\x80\x94 board " +
			   std::to_string(p_spawn.boardSize.x) + "\xC3\x97" + std::to_string(p_spawn.boardSize.y);
	}
}
