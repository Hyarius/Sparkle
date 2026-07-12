#include "structures/voxel/spk_voxel_fluid_simulator.hpp"

#include "structures/voxel/spk_voxel_chunk.hpp"
#include "structures/voxel/spk_voxel_grid.hpp"
#include "structures/voxel/spk_voxel_map.hpp"
#include "structures/voxel/spk_voxel_registry.hpp"

#include <algorithm>
#include <array>
#include <limits>
#include <stdexcept>
#include <tuple>
#include <utility>

namespace
{
	constexpr spk::Vector3Int Down{0, -1, 0};
	constexpr spk::Vector3Int Up{0, 1, 0};
	constexpr std::array<spk::Vector3Int, 4> Sides = {
		spk::Vector3Int{1, 0, 0},
		spk::Vector3Int{-1, 0, 0},
		spk::Vector3Int{0, 0, 1},
		spk::Vector3Int{0, 0, -1}};
}

namespace spk
{
	VoxelFluidSimulator::VoxelFluidSimulator(spk::VoxelMap &p_map, spk::VoxelFluidCellRules p_rules) :
		_map(&p_map),
		_mapLifetime(p_map.lifetimeToken()),
		_rules(std::move(p_rules))
	{
	}

	// ----------------------------------------------------------------------------------
	// Scheduling knobs
	// ----------------------------------------------------------------------------------

	void VoxelFluidSimulator::setTickInterval(long long p_milliseconds) noexcept
	{
		_tickIntervalMs = p_milliseconds;
	}

	long long VoxelFluidSimulator::tickInterval() const noexcept
	{
		return _tickIntervalMs;
	}

	void VoxelFluidSimulator::setMaximumProcessedCells(std::size_t p_maximum) noexcept
	{
		_maximumProcessedCells = p_maximum;
	}

	std::size_t VoxelFluidSimulator::maximumProcessedCells() const noexcept
	{
		return _maximumProcessedCells;
	}

	void VoxelFluidSimulator::setSimulationCenter(std::optional<spk::Vector3Int> p_worldCell) noexcept
	{
		_simulationCenter = std::move(p_worldCell);
	}

	const std::optional<spk::Vector3Int> &VoxelFluidSimulator::simulationCenter() const noexcept
	{
		return _simulationCenter;
	}

	void VoxelFluidSimulator::setHorizontalRange(int p_range)
	{
		if (p_range < 0)
		{
			throw std::invalid_argument("VoxelFluidSimulator horizontal range cannot be negative");
		}
		_horizontalRange = p_range;
	}

	int VoxelFluidSimulator::horizontalRange() const noexcept
	{
		return _horizontalRange;
	}

	std::size_t VoxelFluidSimulator::lastProcessedCellCount() const noexcept
	{
		return _lastProcessedCellCount;
	}

	std::size_t VoxelFluidSimulator::lastChangedCellCount() const noexcept
	{
		return _lastChangedCellCount;
	}

	std::optional<VoxelFluidSimulator::ActiveBox> VoxelFluidSimulator::_activeBox() const noexcept
	{
		if (_simulationCenter.has_value() == false)
		{
			return std::nullopt;
		}
		return ActiveBox{
			.minimum =
				{_simulationCenter->x - _horizontalRange,
				 std::numeric_limits<int>::min(),
				 _simulationCenter->z - _horizontalRange},
			.maximum =
				{_simulationCenter->x + _horizontalRange,
				 std::numeric_limits<int>::max(),
				 _simulationCenter->z + _horizontalRange}};
	}

	namespace
	{
		[[nodiscard]] bool inBox(const auto &p_box, const spk::Vector3Int &p_position)
		{
			if (p_box.has_value() == false)
			{
				return true;
			}
			return p_position.x >= p_box->minimum.x && p_position.x <= p_box->maximum.x &&
				   p_position.y >= p_box->minimum.y && p_position.y <= p_box->maximum.y &&
				   p_position.z >= p_box->minimum.z && p_position.z <= p_box->maximum.z;
		}

		[[nodiscard]] bool chunkIntersectsBox(const spk::Vector3Int &p_chunkCoordinates, const auto &p_box)
		{
			if (p_box.has_value() == false)
			{
				return true;
			}
			const spk::Vector3Int origin = p_chunkCoordinates * spk::VoxelChunk::Size;
			const spk::Vector3Int maximum = origin + spk::VoxelChunk::Size - spk::Vector3Int{1, 1, 1};
			return maximum.x >= p_box->minimum.x && origin.x <= p_box->maximum.x &&
				   maximum.y >= p_box->minimum.y && origin.y <= p_box->maximum.y &&
				   maximum.z >= p_box->minimum.z && origin.z <= p_box->maximum.z;
		}
	}

	// ----------------------------------------------------------------------------------
	// Source tracking
	// ----------------------------------------------------------------------------------

	void VoxelFluidSimulator::_enqueueFrontier(const spk::Vector3Int &p_position)
	{
		if (_frontierMembership.insert(p_position).second)
		{
			_frontier.push_back(p_position);
		}
	}

	std::vector<spk::Vector3Int> VoxelFluidSimulator::_scanChunkSources(const spk::VoxelChunk &p_chunk) const
	{
		const spk::VoxelRegistry &registry = _map->registry();
		std::vector<spk::Vector3Int> sources;
		const spk::Vector3Int origin = p_chunk.worldOrigin();
		const spk::VoxelGrid &grid = p_chunk.grid();
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
					const spk::VoxelFluidRef *reference = registry.tryFluidRef(cell.id);
					if (reference != nullptr && reference->source)
					{
						sources.push_back(origin + spk::Vector3Int{x, y, z});
					}
				}
			}
		}
		return sources;
	}

	void VoxelFluidSimulator::_synchronizeSources()
	{
		const std::vector<spk::Vector3Int> loaded = _map->loadedChunkCoordinates();
		const std::unordered_set<spk::Vector3Int> loadedSet(loaded.begin(), loaded.end());

		// Forget sources whose chunk unloaded; their transient flow unloaded with them.
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

		// Scan chunks not scanned yet for source cells (amortized: only new chunks).
		for (const spk::Vector3Int &coordinates : loaded)
		{
			if (_sourcesByChunk.contains(coordinates))
			{
				continue;
			}
			const spk::VoxelChunk *chunk = _map->tryChunk(coordinates);
			_sourcesByChunk.emplace(
				coordinates,
				chunk != nullptr ? _scanChunkSources(*chunk) : std::vector<spk::Vector3Int>{});
		}
	}

	// ----------------------------------------------------------------------------------
	// Cell classification and placement
	// ----------------------------------------------------------------------------------

	bool VoxelFluidSimulator::_canReceiveFlow(const spk::VoxelCell *p_cell, std::size_t p_level, std::size_t p_family) const
	{
		if (p_cell == nullptr)
		{
			return false; // unloaded cells are never treated as empty
		}
		if (p_cell->isEmpty())
		{
			return true;
		}
		const spk::VoxelFluidRef *reference = _map->registry().tryFluidRef(p_cell->id);
		if (reference != nullptr)
		{
			if (reference->family != p_family || reference->source)
			{
				return false; // other families block; sources are never overwritten
			}
			return reference->level < p_level; // only weaker same-family flow is upgraded
		}
		return _rules.canReplace != nullptr && _rules.canReplace(*p_cell);
	}

	bool VoxelFluidSimulator::_isSupport(const spk::VoxelCell *p_cell) const
	{
		if (p_cell == nullptr || p_cell->isEmpty())
		{
			return false;
		}
		if (_map->registry().tryFluidRef(p_cell->id) != nullptr)
		{
			return false; // fluid below fluid is a column, not support
		}
		return _rules.providesSupport == nullptr || _rules.providesSupport(*p_cell);
	}

	bool VoxelFluidSimulator::_tryPlaceFlow(
		const spk::Vector3Int &p_position,
		const spk::VoxelFluidFamily &p_family,
		std::size_t p_familyIndex,
		std::size_t p_level)
	{
		if (!_canReceiveFlow(_map->tryCell(p_position), p_level, p_familyIndex))
		{
			return false;
		}
		// Generated fluid cells use default orientation and flip.
		if (_map->setCell(p_position, spk::VoxelCell{.id = p_family.level(p_level).runtime}))
		{
			++_lastChangedCellCount;
			return true;
		}
		return false;
	}

	// ----------------------------------------------------------------------------------
	// Propagation
	// ----------------------------------------------------------------------------------

	void VoxelFluidSimulator::_processCell(const spk::Vector3Int &p_position, const std::optional<ActiveBox> &p_box)
	{
		const spk::VoxelRegistry &registry = _map->registry();
		const spk::VoxelCell *self = _map->tryCell(p_position);
		if (self == nullptr || self->isEmpty())
		{
			return;
		}
		const spk::VoxelFluidRef *reference = registry.tryFluidRef(self->id);
		if (reference == nullptr)
		{
			return; // the cell was overwritten by something that is not a fluid
		}
		const spk::VoxelFluidFamily &family = registry.fluidFamily(reference->family);
		const std::size_t maximumLevel = family.levelCount();
		const spk::Vector3Int belowPosition = p_position + Down;

		// 1) Fall straight down while the cell below can receive fluid. A falling column
		//    stays at full strength and does not also spread sideways during that visit.
		if (family.falls && inBox(p_box, belowPosition) &&
			_tryPlaceFlow(belowPosition, family, reference->family, maximumLevel))
		{
			_enqueueFrontier(belowPosition);
			if (reference->source == false)
			{
				_enqueueFrontier(p_position); // keep the column alive until its base settles
			}
			return;
		}

		// 2) Sideways propagation requires whole-cell support: the cell below must be a
		//    loaded, non-empty, non-fluid cell the application classifies as support. A
		//    mid-fall cell (support empty or more of the same fluid) must not bleed the
		//    waterfall outwards at every level; it just sustains the column. Rendered
		//    geometry is never inspected: a slab, slope or stair fills one logical cell.
		const spk::VoxelCell *below = _map->tryCell(belowPosition);
		if (_isSupport(below) == false)
		{
			if (reference->source == false && below == nullptr)
			{
				_enqueueFrontier(p_position); // support not loaded yet; retry once it is
			}
			return;
		}

		// 3) Outgoing strength: sources and waterfall bases (fed by same-family fluid
		//    directly above) pour at full strength; a plain flow cell weakens by one and a
		//    level-one flow cannot spread any farther.
		const spk::VoxelCell *above = _map->tryCell(p_position + Up);
		const spk::VoxelFluidRef *aboveReference =
			(above == nullptr || above->isEmpty()) ? nullptr : registry.tryFluidRef(above->id);
		const bool fedFromAbove = aboveReference != nullptr && aboveReference->family == reference->family;
		const std::size_t outflow = (reference->source || fedFromAbove) ? maximumLevel : reference->level - 1;
		if (outflow < 1)
		{
			return;
		}

		// Gather the candidate destinations. Direct destinations receive fluid in place;
		// when a neighbor cannot receive fluid but is itself a support, the cell above it
		// may receive instead (fluid rising one logical cell onto a neighboring support,
		// still consuming one normal propagation level).
		struct Candidate
		{
			spk::Vector3Int position;
			bool drop = false;
			bool climb = false;
		};
		std::array<Candidate, Sides.size() * 2> candidates{};
		std::size_t candidateCount = 0;
		bool anyDrop = false;
		for (const spk::Vector3Int &offset : Sides)
		{
			const spk::Vector3Int neighborPosition = p_position + offset;
			if (!inBox(p_box, neighborPosition))
			{
				continue;
			}
			const spk::VoxelCell *neighbor = _map->tryCell(neighborPosition);
			if (_canReceiveFlow(neighbor, outflow, reference->family))
			{
				// Drop-priority probe: a direct destination whose own floor can receive
				// fluid is a further fall.
				const spk::Vector3Int neighborBelow = neighborPosition + Down;
				const bool drop = family.falls && inBox(p_box, neighborBelow) &&
								  _canReceiveFlow(_map->tryCell(neighborBelow), maximumLevel, reference->family);
				candidates[candidateCount++] = {.position = neighborPosition, .drop = drop};
				anyDrop = anyDrop || drop;
			}
			else if (_isSupport(neighbor))
			{
				const spk::Vector3Int abovePosition = neighborPosition + Up;
				if (inBox(p_box, abovePosition) &&
					_canReceiveFlow(_map->tryCell(abovePosition), outflow, reference->family))
				{
					candidates[candidateCount++] = {.position = abovePosition, .climb = true};
				}
			}
		}

		// Drop-priority: when at least one direct destination pours over an edge, the
		// water flows only over those edges (no flat spread, no climbing), so a cascade
		// down terraced ground stays a narrow ribbon instead of pooling on every step.
		bool produced = false;
		for (std::size_t index = 0; index < candidateCount; ++index)
		{
			const Candidate &candidate = candidates[index];
			if (anyDrop && candidate.drop == false)
			{
				continue;
			}
			if (_tryPlaceFlow(candidate.position, family, reference->family, outflow))
			{
				_enqueueFrontier(candidate.position);
				produced = true;
			}
		}

		// Flow cells that still push the front stay active; settled ones drop out. Sources
		// are re-enumerated from the source catalog each step, so they need not linger in
		// the frontier.
		if (reference->source == false && produced)
		{
			_enqueueFrontier(p_position);
		}
	}

	// ----------------------------------------------------------------------------------
	// Stepping
	// ----------------------------------------------------------------------------------

	void VoxelFluidSimulator::advance(long long p_deltaMilliseconds)
	{
		_accumulatedMs += p_deltaMilliseconds;
		if (_accumulatedMs < _tickIntervalMs)
		{
			return;
		}
		_accumulatedMs = 0;
		stepNow();
	}

	void VoxelFluidSimulator::stepNow()
	{
		_lastProcessedCellCount = 0;
		_lastChangedCellCount = 0;
		if (hasLiveMap() == false || _map->registry().hasFluids() == false)
		{
			return;
		}
		_synchronizeSources();
		const std::optional<ActiveBox> box = _activeBox();

		// Build a deterministic view of the in-bounds sources: chunks in coordinate order,
		// stable source order within each chunk. The cursor survives across steps, so an
		// over-budget source population is revisited in rotating order.
		std::vector<spk::Vector3Int> sourceChunks;
		sourceChunks.reserve(_sourcesByChunk.size());
		for (const auto &[coordinates, chunkSources] : _sourcesByChunk)
		{
			(void)chunkSources;
			if (chunkIntersectsBox(coordinates, box))
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
				if (inBox(box, position))
				{
					sources.push_back(position);
				}
			}
		}
		if (!sources.empty())
		{
			_sourceCursor %= sources.size();
		}

		// Alternate frontier and source work under the budget: only the frontier entries
		// present when the step began are served (cells re-enqueued during the step wait
		// for the next one), so neither population can starve the other.
		const std::size_t initialFrontierCount = _frontier.size();
		std::size_t frontierVisits = 0;
		std::size_t sourceVisits = 0;
		while (_lastProcessedCellCount < _maximumProcessedCells &&
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
			if (!inBox(box, position))
			{
				continue;
			}
			_processCell(position, box);
		}
	}

	// ----------------------------------------------------------------------------------
	// Dynamic edits
	// ----------------------------------------------------------------------------------

	void VoxelFluidSimulator::notifyCellChanged(const spk::Vector3Int &p_position)
	{
		const spk::VoxelRegistry &registry = _map->registry();
		const spk::VoxelCell *cell = _map->tryCell(p_position);
		const spk::VoxelFluidRef *reference =
			(cell == nullptr || cell->isEmpty()) ? nullptr : registry.tryFluidRef(cell->id);

		// Keep the source catalog current for chunks already scanned; chunks without an
		// entry get scanned wholesale during the next step's synchronization.
		const auto iterator = _sourcesByChunk.find(spk::VoxelChunk::coordinatesFromWorldCell(p_position));
		if (iterator != _sourcesByChunk.end())
		{
			std::vector<spk::Vector3Int> &chunkSources = iterator->second;
			const auto found = std::ranges::find(chunkSources, p_position);
			const bool isSource = reference != nullptr && reference->source;
			if (isSource && found == chunkSources.end())
			{
				chunkSources.push_back(p_position);
			}
			else if (!isSource && found != chunkSources.end())
			{
				chunkSources.erase(found);
			}
		}

		// Wake the neighborhood: the edited cell itself if it holds flow, plus adjacent
		// fluid that may now flow into (or through) the edited position.
		if (reference != nullptr && reference->source == false)
		{
			_enqueueFrontier(p_position);
		}
		constexpr std::array<spk::Vector3Int, 6> Neighbors = {
			spk::Vector3Int{1, 0, 0},
			spk::Vector3Int{-1, 0, 0},
			spk::Vector3Int{0, 1, 0},
			spk::Vector3Int{0, -1, 0},
			spk::Vector3Int{0, 0, 1},
			spk::Vector3Int{0, 0, -1}};
		for (const spk::Vector3Int &offset : Neighbors)
		{
			const spk::Vector3Int neighborPosition = p_position + offset;
			const spk::VoxelCell *neighbor = _map->tryCell(neighborPosition);
			if (neighbor == nullptr || neighbor->isEmpty())
			{
				continue;
			}
			const spk::VoxelFluidRef *neighborReference = registry.tryFluidRef(neighbor->id);
			if (neighborReference != nullptr && neighborReference->source == false)
			{
				_enqueueFrontier(neighborPosition);
			}
		}
	}

	void VoxelFluidSimulator::rescanChunk(const spk::Vector3Int &p_chunkCoordinates)
	{
		const spk::VoxelChunk *chunk = _map->tryChunk(p_chunkCoordinates);
		if (chunk == nullptr)
		{
			_sourcesByChunk.erase(p_chunkCoordinates);
			return;
		}
		_sourcesByChunk[p_chunkCoordinates] = _scanChunkSources(*chunk);
	}

	void VoxelFluidSimulator::reset()
	{
		_sourcesByChunk.clear();
		_frontier.clear();
		_frontierMembership.clear();
		_sourceCursor = 0;
		_serveFrontierNext = true;
	}

	std::size_t VoxelFluidSimulator::pendingCellCount() const noexcept
	{
		return _frontier.size();
	}

	// ----------------------------------------------------------------------------------
	// Map lifetime
	// ----------------------------------------------------------------------------------

	bool VoxelFluidSimulator::hasLiveMap() const noexcept
	{
		return _mapLifetime.expired() == false;
	}

	std::weak_ptr<const void> VoxelFluidSimulator::mapLifetime() const noexcept
	{
		return _mapLifetime;
	}

	spk::VoxelMap &VoxelFluidSimulator::map()
	{
		if (hasLiveMap() == false)
		{
			throw std::logic_error("VoxelFluidSimulator map has been destroyed");
		}
		return *_map;
	}

	const spk::VoxelMap &VoxelFluidSimulator::map() const
	{
		if (hasLiveMap() == false)
		{
			throw std::logic_error("VoxelFluidSimulator map has been destroyed");
		}
		return *_map;
	}
}
