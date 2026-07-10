#pragma once

#include "components/actor.hpp"

#include "structures/game_engine/spk_component_logic.hpp"
#include "structures/math/spk_vector3.hpp"

#include <cstddef>
#include <optional>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace pg
{
	class VoxelWorld;
	class VoxelRegistry;

	// App-side cellular automaton that makes worldgen-placed fluid sources spread and fall.
	//
	// It works entirely through the voxel map's public cell API, so every edit re-bakes the
	// affected chunk exactly like any other cell change. It only ever touches already-loaded
	// chunks near the player and never forces a chunk to generate. Sources are persistent (they
	// come from worldgen); the flow it writes is transient - it is not persisted and is simply
	// re-derived whenever a chunk reloads.
	//
	// The logic is templated on Actor only to ride the engine's update pass and learn the
	// player's position; the simulation itself operates on the whole map in _executeUpdate.
	class FluidSimulationLogic : public spk::ComponentLogic<Actor>
	{
	private:
		VoxelWorld &_world;
		const VoxelRegistry &_registry;

		long long _tickIntervalMs = 120;     // one wavefront step every ~120 ms (visible flow, low churn)
		long long _accumulatedMs = 0;
		int _range = 48;                      // half-extent (blocks, horizontal) of the simulated box
		std::size_t _maxCellsPerTick = 6000;  // hard cap on cells advanced per step

		std::optional<spk::Vector3Int> _playerCell;

		// Source cells discovered per loaded chunk (so an unload drops them all in one erase),
		// plus the moving flow front to advance on the next step.
		std::unordered_map<spk::Vector3Int, std::vector<spk::Vector3Int>> _sourcesByChunk;
		std::unordered_set<spk::Vector3Int> _frontier;

		void _syncLoadedChunks();
		void _step();
		[[nodiscard]] bool _inRange(const spk::Vector3Int &p_cell) const;

	public:
		explicit FluidSimulationLogic(VoxelWorld &p_world);

	protected:
		void _parseComponentForUpdate(const spk::UpdateContext &p_tick, Actor &p_actor) override;
		void _executeUpdate(const spk::UpdateContext &p_tick) override;
	};
}
