#include "logics/fluid_simulation_logic.hpp"

#include "voxel/voxel_registry.hpp"
#include "world/voxel_world.hpp"

#include "structures/voxel/spk_voxel_chunk.hpp"
#include "structures/voxel/spk_voxel_grid.hpp"
#include "structures/voxel/spk_voxel_map.hpp"

#include <algorithm>
#include <array>
#include <cstdlib>
#include <tuple>
#include <utility>
#include <vector>

namespace pg
{
	FluidSimulationLogic::FluidSimulationLogic(VoxelWorld &p_world) :
		_world(p_world),
		_registry(p_world.registry())
	{
		// Between the chunk streamer (priority 100, loads chunks) and the render logic
		// (priority 0, bakes them): our edits this step are baked in the same frame.
		setPriority(50);
	}

	void FluidSimulationLogic::_parseComponentForUpdate(const spk::UpdateContext &, Actor &p_actor)
	{
		if (p_actor.player)
		{
			setSimulationCenter(p_actor.cell);
		}
	}

	void FluidSimulationLogic::_executeUpdate(const spk::UpdateContext &p_tick)
	{
		if (_registry.fluids().empty())
		{
			return;
		}

		_accumulatedMs += static_cast<long long>(p_tick.deltaTime.milliseconds());
		if (_accumulatedMs < _tickIntervalMs)
		{
			return;
		}
		_accumulatedMs = 0;

		stepNow();
	}

	void FluidSimulationLogic::setSimulationCenter(std::optional<spk::Vector3Int> p_center) noexcept
	{
		_playerCell = std::move(p_center);
	}

	void FluidSimulationLogic::setMaxCellsPerTick(std::size_t p_maximum) noexcept
	{
		_maxCellsPerTick = p_maximum;
	}

	std::size_t FluidSimulationLogic::maxCellsPerTick() const noexcept
	{
		return _maxCellsPerTick;
	}

	std::size_t FluidSimulationLogic::lastProcessedCellCount() const noexcept
	{
		return _lastProcessedCellCount;
	}

	void FluidSimulationLogic::stepNow()
	{
		if (_registry.fluids().empty())
		{
			_lastProcessedCellCount = 0;
			return;
		}
		_syncLoadedChunks();
		_step();
	}

	bool FluidSimulationLogic::_inRange(const spk::Vector3Int &p_cell) const
	{
		if (_playerCell.has_value() == false)
		{
			return true;
		}
		const spk::Vector3Int delta = p_cell - *_playerCell;
		return std::abs(delta.x) <= _range && std::abs(delta.z) <= _range;
	}

	void FluidSimulationLogic::_syncLoadedChunks()
	{
		spk::VoxelMap &map = _world.map();
		const std::vector<spk::Vector3Int> loaded = map.loadedChunkCoordinates();
		const std::unordered_set<spk::Vector3Int> loadedSet(loaded.begin(), loaded.end());

		// Forget sources whose chunk unloaded; their transient flow is regenerated away anyway.
		for (auto iterator = _sourcesByChunk.begin(); iterator != _sourcesByChunk.end();)
		{
			if (loadedSet.contains(iterator->first))
			{
				++iterator;
			}
			else
			{
				iterator = _sourcesByChunk.erase(iterator);
			}
		}

		// Scan chunks not scanned yet for fluid source cells (amortized: only new chunks).
		for (const spk::Vector3Int &coordinates : loaded)
		{
			if (_sourcesByChunk.contains(coordinates))
			{
				continue;
			}

			std::vector<spk::Vector3Int> sources;
			const spk::VoxelChunk *chunk = map.tryChunk(coordinates);
			if (chunk != nullptr)
			{
				const spk::Vector3Int origin = chunk->worldOrigin();
				const spk::VoxelGrid &grid = chunk->grid();
				const spk::Vector3Int size = grid.size();
				for (int y = 0; y < size.y; ++y)
				{
					for (int z = 0; z < size.z; ++z)
					{
						for (int x = 0; x < size.x; ++x)
						{
							const spk::VoxelCell &cell = grid.cell(x, y, z);
							if (cell.isEmpty())
							{
								continue;
							}
							const FluidRef *reference = _registry.tryFluidRef(cell.id);
							if (reference != nullptr && reference->source)
							{
								sources.push_back(origin + spk::Vector3Int{x, y, z});
							}
						}
					}
				}
			}
			_sourcesByChunk.emplace(coordinates, std::move(sources));
		}
	}

	void FluidSimulationLogic::_enqueueFrontier(const spk::Vector3Int &p_position)
	{
		if (_frontierMembership.insert(p_position).second)
		{
			_frontier.push_back(p_position);
		}
	}

	void FluidSimulationLogic::_step()
	{
		spk::VoxelMap &map = _world.map();
		const std::vector<FluidFamily> &families = _registry.fluids();

		static constexpr spk::Vector3Int Down{0, -1, 0};
		static constexpr spk::Vector3Int Up{0, 1, 0};
		static constexpr std::array<spk::Vector3Int, 4> Sides = {
			spk::Vector3Int{1, 0, 0},
			spk::Vector3Int{-1, 0, 0},
			spk::Vector3Int{0, 0, 1},
			spk::Vector3Int{0, 0, -1}};

		// Build a deterministic view of only the in-range source chunks. The cursor survives
		// across steps, so an over-budget source population is revisited in rotating order.
		std::vector<spk::Vector3Int> sourceChunks;
		sourceChunks.reserve(_sourcesByChunk.size());
		for (const auto &[coordinates, sources] : _sourcesByChunk)
		{
			(void)sources;
			const spk::Vector3Int origin = coordinates * spk::VoxelChunk::Size;
			const spk::Vector3Int maximum = origin + spk::VoxelChunk::Size - spk::Vector3Int{1, 1, 1};
			if (!_playerCell.has_value() ||
				(maximum.x >= _playerCell->x - _range && origin.x <= _playerCell->x + _range &&
				 maximum.z >= _playerCell->z - _range && origin.z <= _playerCell->z + _range))
			{
				sourceChunks.push_back(coordinates);
			}
		}
		std::ranges::sort(sourceChunks, [](const spk::Vector3Int &p_left, const spk::Vector3Int &p_right) {
			return std::tie(p_left.x, p_left.y, p_left.z) < std::tie(p_right.x, p_right.y, p_right.z);
		});
		std::vector<spk::Vector3Int> sources;
		for (const spk::Vector3Int &coordinates : sourceChunks)
		{
			for (const spk::Vector3Int &position : _sourcesByChunk.at(coordinates))
			{
				if (_inRange(position))
				{
					sources.push_back(position);
				}
			}
		}
		if (!sources.empty())
		{
			_sourceCursor %= sources.size();
		}

		const std::size_t initialFrontierCount = _frontier.size();
		std::size_t frontierVisits = 0;
		std::size_t sourceVisits = 0;
		_lastProcessedCellCount = 0;
		while (_lastProcessedCellCount < _maxCellsPerTick &&
			   (frontierVisits < initialFrontierCount || sourceVisits < sources.size()))
		{
			const bool frontierAvailable = frontierVisits < initialFrontierCount;
			const bool sourceAvailable = sourceVisits < sources.size();
			const bool useFrontier = frontierAvailable && (!sourceAvailable || _serveFrontierNext);
			spk::Vector3Int position;
			if (useFrontier)
			{
				position = _frontier.front();
				_frontier.pop_front();
				_frontierMembership.erase(position);
				++frontierVisits;
				_serveFrontierNext = false;
			}
			else
			{
				position = sources[_sourceCursor];
				_sourceCursor = (_sourceCursor + 1) % sources.size();
				++sourceVisits;
				_serveFrontierNext = true;
			}
			++_lastProcessedCellCount;
			if (!_inRange(position))
			{
				continue;
			}

			const spk::VoxelCell *self = map.tryCell(position);
			if (self == nullptr || self->isEmpty())
			{
				continue;
			}
			const FluidRef *reference = _registry.tryFluidRef(self->id);
			if (reference == nullptr)
			{
				continue; // the cell was overwritten by something that is not this fluid
			}
			const FluidFamily &family = families[reference->family];
			const spk::Vector3Int belowPosition = position + Down;
			const spk::VoxelCell *below = map.tryCell(belowPosition);

			// 1) Fall straight down into empty space. A falling column stays full and does not
			//    also spread sideways while it can still descend.
			if (family.falls && below != nullptr && below->isEmpty())
			{
				map.setCell(belowPosition, spk::VoxelCell{.id = family.stageId(family.maxSpread)});
				_enqueueFrontier(belowPosition);
				if (reference->source == false)
				{
					_enqueueFrontier(position); // keep the column alive until its base is supported
				}
				continue;
			}

			// Only fluid resting on solid ground spreads sideways. A mid-fall cell - one whose
			// support is empty or is more of the same fluid - must not bleed the waterfall
			// outwards at every level; it just sustains the column. Sources follow the same
			// rule once their fall has started, so elevated source probes do not bloom into a
			// blocky tower at the top.
			const bool restsOnSolid = below != nullptr && below->isEmpty() == false &&
									  _registry.tryFluidRef(below->id) == nullptr;
			if (restsOnSolid == false)
			{
				if (reference->source == false && below == nullptr)
				{
					_enqueueFrontier(position); // support not loaded yet; retry once it is
				}
				continue;
			}

			// 2) Spread sideways one stage weaker. Falling resets the flow: a source or a
			//    waterfall's base (fed by fluid directly above) pours at full strength, so the
			//    floor below a fall gets a full-length pool; a plain flow cell weakens by one
			//    and dies after maxSpread cells.
			const spk::VoxelCell *above = map.tryCell(position + Up);
			const FluidRef *aboveReference = above == nullptr ? nullptr : _registry.tryFluidRef(above->id);
			const bool fedFromAbove = aboveReference != nullptr && aboveReference->family == reference->family;
			const int outflow = (reference->source || fedFromAbove) ? family.maxSpread : reference->stage - 1;

			bool produced = false;
			if (outflow >= 1)
			{
				// Drop-priority: an open side whose own floor is empty is a further fall. When one
				// exists the water pours only over those edges instead of flooding the flat, so a
				// cascade down terraced ground stays a narrow ribbon (at most one block sideways
				// per step) rather than growing a full pool on every step.
				std::array<spk::Vector3Int, Sides.size()> openNeighbors{};
				std::array<bool, Sides.size()> openIsDrop{};
				std::size_t openCount = 0;
				bool anyDrop = false;
				for (const spk::Vector3Int &offset : Sides)
				{
					const spk::Vector3Int neighborPosition = position + offset;
					if (_inRange(neighborPosition) == false)
					{
						continue;
					}
					const spk::VoxelCell *neighbor = map.tryCell(neighborPosition);
					if (neighbor == nullptr || neighbor->isEmpty() == false)
					{
						continue; // not loaded, or already occupied (no upgrades in this first version)
					}
					const spk::VoxelCell *neighborFloor = map.tryCell(neighborPosition + Down);
					const bool drop = neighborFloor != nullptr && neighborFloor->isEmpty();
					openNeighbors[openCount] = neighborPosition;
					openIsDrop[openCount] = drop;
					anyDrop = anyDrop || drop;
					++openCount;
				}
				for (std::size_t index = 0; index < openCount; ++index)
				{
					if (anyDrop && openIsDrop[index] == false)
					{
						continue;
					}
					map.setCell(openNeighbors[index], spk::VoxelCell{.id = family.stageId(outflow)});
					_enqueueFrontier(openNeighbors[index]);
					produced = true;
				}
			}

			// Flow cells that still push the front stay active; settled ones drop out. Sources
			// are re-enumerated from _sourcesByChunk each step, so they need not linger here.
			if (reference->source == false && produced)
			{
				_enqueueFrontier(position);
			}
		}
	}
}
