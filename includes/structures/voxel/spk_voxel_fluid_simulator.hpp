#pragma once

#include "structures/game_engine/spk_component.hpp"
#include "structures/math/spk_vector3.hpp"
#include "structures/voxel/spk_voxel_cell.hpp"
#include "structures/voxel/spk_voxel_fluid.hpp"

#include <cstddef>
#include <deque>
#include <functional>
#include <memory>
#include <optional>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace spk
{
	class VoxelChunk;
	class VoxelMap;

	// Application policy over cells the engine cannot classify itself. Both callbacks are
	// invoked only for non-empty, NON-FLUID cells: the simulator already knows how to
	// handle empty cells and every fluid state (same family, other family, source, flow),
	// so the callbacks never have to classify fluids.
	//
	// When canReplace is missing no gameplay voxel is ever replaced by fluid; when
	// providesSupport is missing every non-empty non-fluid cell counts as support.
	struct VoxelFluidCellRules
	{
		// May incoming fluid overwrite this cell (e.g. grass, bushes)?
		std::function<bool(const spk::VoxelCell &)> canReplace;

		// Does this cell support fluid resting on it (a floor to spread from)?
		std::function<bool(const spk::VoxelCell &)> providesSupport;
	};

	// Deterministic cellular fluid automaton over one VoxelMap, packaged as a regular game
	// component: attach it to any entity (typically the player, whose position re-emits
	// setSimulationCenter), hand it the application's cell rules, and the
	// VoxelFluidSimulationLogic steps it on the update pass between chunk streaming and
	// mesh baking. Persistent sources (state 0 of a fluid family) spread and fall as
	// generated flow states written straight into the map, so every edit re-bakes the
	// affected chunk exactly like any other cell change. It is a gameplay system, not a
	// physical solver: no pressure, no volume conservation, no retraction - generated flow
	// is additive while its chunks stay loaded and is re-derived when a chunk reloads.
	//
	// Scheduling is deterministic and fair: source chunks are visited in coordinate order
	// with a rotating cursor that survives across steps, and source work alternates with an
	// insertion-ordered frontier queue so a large source population can never starve flow
	// cells (and vice versa) under the cell budget.
	//
	// Threading: stepping and the dynamic-edit notifications mutate the map (or state
	// derived from it) and must run on the map's owning update thread, after the chunk
	// streamer has bound mutation ownership (see VoxelMap::bindMutationThread). The
	// simulator never creates worker threads and never loads chunks.
	class VoxelFluidSimulator final : public spk::Component
	{
	public:
		static constexpr long long DefaultTickIntervalMs = 120; // visible flow, low churn
		static constexpr std::size_t DefaultMaximumProcessedCells = 6000;
		static constexpr int DefaultHorizontalRange = 48; // half-extent in cells around the center

	private:
		// Inclusive world-cell box derived from the simulation center and horizontal range.
		struct ActiveBox
		{
			spk::Vector3Int minimum;
			spk::Vector3Int maximum;
		};

		spk::VoxelMap *_map = nullptr;
		std::weak_ptr<const void> _mapLifetime;
		spk::VoxelFluidCellRules _rules;

		long long _tickIntervalMs = DefaultTickIntervalMs;
		long long _accumulatedMs = 0;
		std::size_t _maximumProcessedCells = DefaultMaximumProcessedCells;
		int _horizontalRange = DefaultHorizontalRange;
		std::optional<spk::Vector3Int> _simulationCenter;
		std::size_t _lastProcessedCellCount = 0;
		std::size_t _lastChangedCellCount = 0;

		// Source cells discovered per loaded chunk (an unload drops them in one erase).
		// The frontier is an insertion-ordered queue with a membership set: processed
		// cells move to the back only when they still need service, so no rebuilt source
		// prefix can permanently overtake them.
		std::unordered_map<spk::Vector3Int, std::vector<spk::Vector3Int>> _sourcesByChunk;
		std::deque<spk::Vector3Int> _frontier;
		std::unordered_set<spk::Vector3Int> _frontierMembership;
		std::size_t _sourceCursor = 0;
		bool _serveFrontierNext = true;

		[[nodiscard]] std::optional<ActiveBox> _activeBox() const noexcept;
		void _synchronizeSources();
		[[nodiscard]] std::vector<spk::Vector3Int> _scanChunkSources(const spk::VoxelChunk &p_chunk) const;
		void _enqueueFrontier(const spk::Vector3Int &p_position);
		[[nodiscard]] bool _canReceiveFlow(const spk::VoxelCell *p_cell, std::size_t p_level, std::size_t p_family) const;
		[[nodiscard]] bool _isSupport(const spk::VoxelCell *p_cell) const;
		bool _tryPlaceFlow(
			const spk::Vector3Int &p_position,
			const spk::VoxelFluidFamily &p_family,
			std::size_t p_familyIndex,
			std::size_t p_level);
		void _processCell(const spk::Vector3Int &p_position, const std::optional<ActiveBox> &p_box);

	public:
		VoxelFluidSimulator(spk::VoxelMap &p_map, spk::VoxelFluidCellRules p_rules);

		// One wavefront step every p_milliseconds of accumulated update time; zero or a
		// negative interval steps on every update.
		void setTickInterval(long long p_milliseconds) noexcept;
		[[nodiscard]] long long tickInterval() const noexcept;

		// Hard cap on cells serviced during one step (sources + frontier combined).
		void setMaximumProcessedCells(std::size_t p_maximum) noexcept;
		[[nodiscard]] std::size_t maximumProcessedCells() const noexcept;

		// World cell the simulated box is centred on; without one the whole loaded map is
		// simulated. The box spans the horizontal range on x/z and is unbounded vertically,
		// so falling columns keep falling however deep the world goes. Chunks are never
		// force-loaded either way.
		void setSimulationCenter(std::optional<spk::Vector3Int> p_worldCell) noexcept;
		[[nodiscard]] const std::optional<spk::Vector3Int> &simulationCenter() const noexcept;
		void setHorizontalRange(int p_range);
		[[nodiscard]] int horizontalRange() const noexcept;

		[[nodiscard]] std::size_t lastProcessedCellCount() const noexcept;
		[[nodiscard]] std::size_t lastChangedCellCount() const noexcept;

		// Accumulates update time and runs stepNow() once the tick interval has elapsed.
		// Called by VoxelFluidSimulationLogic; must run on the map's update thread.
		void advance(long long p_deltaMilliseconds);
		// Runs one budgeted scheduling/simulation pass immediately: synchronizes the source
		// catalog with the currently loaded chunks, then alternately services frontier
		// cells and sources until the budget or the pending work runs out (tools/tests).
		void stepNow();

		// Tells the simulator one world cell changed outside of its own writes: newly
		// placed sources are tracked, deleted sources forgotten, and the surrounding fluid
		// re-enqueued so it reacts to the edit.
		void notifyCellChanged(const spk::Vector3Int &p_position);

		// Rebuilds one chunk's source list immediately (drops it if the chunk is unloaded).
		void rescanChunk(const spk::Vector3Int &p_chunkCoordinates);

		// Forgets every discovered source and pending frontier cell.
		void reset();

		[[nodiscard]] std::size_t pendingCellCount() const noexcept;

		// VoxelMap is borrowed rather than owned. Access after the map has been destroyed
		// fails deterministically instead of dereferencing a dangling pointer.
		[[nodiscard]] bool hasLiveMap() const noexcept;
		[[nodiscard]] std::weak_ptr<const void> mapLifetime() const noexcept;
		[[nodiscard]] spk::VoxelMap &map();
		[[nodiscard]] const spk::VoxelMap &map() const;
	};
}
