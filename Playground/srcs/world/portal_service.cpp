#include "world/portal_service.hpp"

#include "battle/battle_context.hpp"
#include "components/actor.hpp"
#include "core/registries.hpp"
#include "creatures/creature_unit.hpp"
#include "encounters/biome.hpp"
#include "world/map_definition.hpp"
#include "world/voxel_world.hpp"
#include "world/world_navigation.hpp"
#include "voxel/voxel_traversal.hpp"

#include <algorithm>
#include <cmath>
#include <stdexcept>
#include <utility>

namespace
{
	[[nodiscard]] int horizontalDistance(const spk::Vector3Int &p_left, const spk::Vector3Int &p_right)
	{
		return std::abs(p_left.x - p_right.x) + std::abs(p_left.z - p_right.z);
	}
}

namespace pg
{
	PortalService::PortalService(
		GameContext &p_context,
		const Registries &p_registries,
		Actor &p_player,
		RelocateFunction p_relocate,
		TransitionFunction p_transitioned) :
		_context(p_context),
		_registries(p_registries),
		_player(p_player),
		_relocate(std::move(p_relocate)),
		_transitioned(std::move(p_transitioned)),
		_moveContract(_context.events.playerMoved.subscribe([this](spk::Vector3Int p_cell) {
			_onPlayerMoved(p_cell);
		})),
		_battleResolvedContract(_context.events.battleResolved.subscribe(
			[this](BattleContext *, BattleSide p_winner) {
				if (p_winner == BattleSide::Player)
				{
					return;
				}
				healParty();
				_respawnPending = _context.respawnPoint.has_value();
				if (_respawnPending && _context.world.explorationActive)
				{
					transitionTo(*_context.respawnPoint);
					_respawnPending = false;
				}
			})),
		_battleEndContract(_context.events.battleEndConfirmed.subscribe([this] {
			if (_respawnPending && _context.respawnPoint.has_value())
			{
				transitionTo(*_context.respawnPoint);
				_respawnPending = false;
			}
		}))
	{
	}

	const MapPortal &PortalService::_requirePortal(
		const std::string &p_mapId, const std::string &p_portalName) const
	{
		const MapDefinition &map = _registries.maps().get(p_mapId);
		const auto portal = std::ranges::find(map.portals, p_portalName, &MapPortal::name);
		if (portal == map.portals.end())
		{
			throw std::out_of_range(
				"map '" + p_mapId + "' has no portal named '" + p_portalName + "'");
		}
		return *portal;
	}

	void PortalService::transitionToPortal(const std::string &p_mapId, const std::string &p_portalName)
	{
		const MapPortal &portal = _requirePortal(p_mapId, p_portalName);
		transitionTo(WorldPosition{.map = p_mapId, .cell = portal.at});
	}

	void PortalService::transitionTo(const WorldPosition &p_position)
	{
		if (_transitioning || _context.world.world == nullptr || _context.world.navigation == nullptr)
		{
			return;
		}
		_transitioning = true;
		try
		{
			const MapDefinition &map = _registries.maps().get(p_position.map);
			if (!isStandable(map.grid, p_position.cell, _registries.voxels()))
			{
				throw std::runtime_error(
					"portal destination in map '" + p_position.map + "' is not standable");
			}
			_context.world.world->activateCachedMap(map);
			_context.world.navigation->resetBounds(TraversalBounds{{0, 0, 0}, map.size()});
			_context.world.navigation->refresh();
			_context.world.activeMap = &map;
			_context.world.activeBiome = map.biome.empty() ? nullptr : &_registries.biomes().get(map.biome);
			_player.cell = p_position.cell;
			_player.path.clear();
			_player.segment = 0;
			_player.segmentProgress = 0.0f;
			if (_relocate) _relocate(_player);
			if (_transitioned) _transitioned();
			_context.events.worldChanged.trigger();
			_refreshPrompt(_player.cell);
		}
		catch (...)
		{
			_transitioning = false;
			throw;
		}
		_transitioning = false;
	}

	void PortalService::healParty()
	{
		for (std::unique_ptr<CreatureUnit> &creature : _context.player.team)
		{
			if (creature != nullptr)
			{
				creature->currentHealth = creature->attributes.health;
			}
		}
		_context.events.partyChanged.trigger();
	}

	bool PortalService::atHealPoint(const spk::Vector3Int &p_cell) const
	{
		if (_context.world.activeMap == nullptr) return false;
		return std::ranges::any_of(_context.world.activeMap->markers, [&](const MapMarker &p_marker) {
			return p_marker.name.starts_with("healPoint") &&
				p_marker.at.x == p_cell.x && p_marker.at.z == p_cell.z;
		});
	}

	void PortalService::_refreshPrompt(const spk::Vector3Int &p_cell)
	{
		std::string prompt;
		if (_context.world.activeMap != nullptr)
		{
			const bool nearPortal = std::ranges::any_of(
				_context.world.activeMap->portals, [&](const MapPortal &p_portal) {
					return horizontalDistance(p_portal.at, p_cell) <= 1;
				});
			const bool nearHeal = std::ranges::any_of(
				_context.world.activeMap->markers, [&](const MapMarker &p_marker) {
					return p_marker.name.starts_with("healPoint") && horizontalDistance(p_marker.at, p_cell) <= 1;
				});
			if (nearPortal) prompt = "Enter";
			else if (nearHeal) prompt = "Heal team";
		}
		_context.events.interactionPromptChanged.trigger(std::move(prompt));
	}

	void PortalService::_onPlayerMoved(const spk::Vector3Int &p_cell)
	{
		if (_transitioning || !_context.world.explorationActive || _context.world.activeMap == nullptr)
		{
			return;
		}
		if (atHealPoint(p_cell))
		{
			healParty();
			_context.respawnPoint = WorldPosition{.map = _context.world.activeMap->id, .cell = p_cell};
		}
		const auto portal = std::ranges::find_if(
			_context.world.activeMap->portals, [&](const MapPortal &p_portal) {
				return p_portal.at == p_cell;
			});
		if (portal != _context.world.activeMap->portals.end())
		{
			transitionToPortal(portal->target.map, portal->target.portal);
			return;
		}
		_refreshPrompt(p_cell);
	}
}
