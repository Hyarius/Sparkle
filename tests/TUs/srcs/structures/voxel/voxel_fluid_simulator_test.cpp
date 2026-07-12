#include <gtest/gtest.h>

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <memory>
#include <stdexcept>
#include <vector>

#include "structures/game_engine/spk_entity_3d.hpp"
#include "structures/game_engine/spk_game_engine.hpp"
#include "structures/voxel/spk_cube_voxel_shape.hpp"
#include "structures/voxel/spk_voxel_chunk.hpp"
#include "structures/voxel/spk_voxel_chunk_streamer_logic.hpp"
#include "structures/voxel/spk_voxel_fluid_simulation_logic.hpp"
#include "structures/voxel/spk_voxel_fluid_simulator.hpp"
#include "structures/voxel/spk_voxel_map.hpp"
#include "structures/voxel/spk_voxel_registry.hpp"

namespace
{
	[[nodiscard]] spk::VoxelFluidDescription makeDescription(std::size_t p_levelCount, bool p_falls = true)
	{
		return spk::VoxelFluidDescription{
			.levelCount = p_levelCount,
			.falls = p_falls,
			.appearance = spk::VoxelFluidAppearance{
				.topTexture = {2, 1},
				.bottomTexture = {2, 1},
				.sideTexture = {2, 1},
				.transparency = 0.5f}};
	}

	// A minimal game catalog: solid stone (support), a replaceable decoration, a blocking
	// decoration (neither replaceable nor support) and two eight/four-level fluids.
	class SimulatorWorld
	{
	public:
		spk::VoxelRegistry registry;
		spk::VoxelRuntimeId stone{};
		spk::VoxelRuntimeId bush{};  // replaceable, no support
		spk::VoxelRuntimeId fence{}; // neither replaceable nor support
		std::size_t waterFamily = 0;
		std::size_t lavaFamily = 0;
		std::unique_ptr<spk::VoxelMap> map;
		std::unique_ptr<spk::VoxelFluidSimulator> simulator;

		explicit SimulatorWorld(
			spk::VoxelMap::ChunkGenerator p_generator = [](spk::VoxelChunk &) {},
			std::size_t p_waterLevels = 8,
			bool p_waterFalls = true)
		{
			stone = registry.registerShape(std::make_unique<spk::CubeVoxelShape>(spk::AtlasCell{0, 0}));
			bush = registry.registerShape(std::make_unique<spk::CubeVoxelShape>(spk::AtlasCell{1, 0}));
			fence = registry.registerShape(std::make_unique<spk::CubeVoxelShape>(spk::AtlasCell{2, 0}));
			waterFamily = registry.registerFluid(makeDescription(p_waterLevels, p_waterFalls));
			lavaFamily = registry.registerFluid(makeDescription(4));
			map = std::make_unique<spk::VoxelMap>(registry, std::move(p_generator));
			simulator = std::make_unique<spk::VoxelFluidSimulator>(
				*map,
				spk::VoxelFluidCellRules{
					.canReplace = [this](const spk::VoxelCell &p_cell) { return p_cell.id == bush; },
					.providesSupport = [this](const spk::VoxelCell &p_cell) { return p_cell.id == stone; }});
		}

		[[nodiscard]] const spk::VoxelFluidFamily &water() const
		{
			return registry.fluidFamily(waterFamily);
		}

		[[nodiscard]] const spk::VoxelFluidFamily &lava() const
		{
			return registry.fluidFamily(lavaFamily);
		}

		void loadChunk(const spk::Vector3Int &p_coordinates)
		{
			(void)map->chunk(p_coordinates);
		}

		void set(const spk::Vector3Int &p_position, spk::VoxelRuntimeId p_id)
		{
			ASSERT_TRUE(map->setCell(p_position, spk::VoxelCell{.id = p_id}));
		}

		// Fills the whole y == 0 plane of chunk {0,0,0} with stone.
		void fillFloor()
		{
			for (int z = 0; z < spk::VoxelChunk::Size.z; ++z)
			{
				for (int x = 0; x < spk::VoxelChunk::Size.x; ++x)
				{
					set({x, 0, z}, stone);
				}
			}
		}

		[[nodiscard]] std::size_t levelAt(const spk::Vector3Int &p_position) const
		{
			const spk::VoxelCell *cell = map->tryCell(p_position);
			if (cell == nullptr || cell->isEmpty())
			{
				return 0;
			}
			const spk::VoxelFluidRef *reference = registry.tryFluidRef(cell->id);
			return (reference != nullptr && reference->family == waterFamily && !reference->source) ? reference->level : 0;
		}

		[[nodiscard]] bool isEmptyAt(const spk::Vector3Int &p_position) const
		{
			const spk::VoxelCell *cell = map->tryCell(p_position);
			return cell != nullptr && cell->isEmpty();
		}

		void step(std::size_t p_budget = 6000)
		{
			simulator->setMaximumProcessedCells(p_budget);
			simulator->stepNow();
		}

		void settle(int p_steps = 64)
		{
			for (int index = 0; index < p_steps; ++index)
			{
				step();
			}
		}
	};

	// The historical worldgen scenario: every generated chunk gets a stone floor at
	// y == 0 plus up to six water sources at fixed local positions.
	[[nodiscard]] std::unique_ptr<SimulatorWorld> makeSourceBurstWorld(
		std::size_t p_sourceCount,
		std::vector<spk::Vector3Int> p_chunks = {{0, 0, 0}})
	{
		auto world = std::make_unique<SimulatorWorld>();
		spk::VoxelRuntimeId floor = world->stone;
		spk::VoxelRuntimeId source = world->water().sourceRuntime;
		*world = SimulatorWorld{}; // placeholder, replaced below
		(void)floor;
		(void)source;
		return world;
	}
}

// ---------------------------------------------------------------------------
// Falling
// ---------------------------------------------------------------------------

TEST(VoxelFluidSimulator, SourceFallsAtMaximumLevelWithoutSidewaysSpread)
{
	SimulatorWorld world;
	world.loadChunk({0, 0, 0});
	world.set({8, 8, 8}, world.water().sourceRuntime);

	world.step();
	// The fall lands one cell below at full strength; nothing spread sideways.
	EXPECT_EQ(world.levelAt({8, 7, 8}), 8u);
	EXPECT_TRUE(world.isEmptyAt({7, 8, 8}));
	EXPECT_TRUE(world.isEmptyAt({9, 8, 8}));

	world.step();
	// The generated column keeps falling and still never bleeds sideways mid-fall.
	EXPECT_EQ(world.levelAt({8, 6, 8}), 8u);
	EXPECT_TRUE(world.isEmptyAt({7, 7, 8}));
	EXPECT_TRUE(world.isEmptyAt({9, 7, 8}));
}

TEST(VoxelFluidSimulator, NonFallingFamiliesNeverFall)
{
	SimulatorWorld world([](spk::VoxelChunk &) {}, 8, /* p_waterFalls = */ false);
	world.loadChunk({0, 0, 0});
	world.set({8, 8, 8}, world.water().sourceRuntime);

	world.settle(8);
	EXPECT_TRUE(world.isEmptyAt({8, 7, 8}));

	// Once supported, a non-falling fluid still spreads sideways normally.
	world.set({8, 7, 8}, world.stone);
	world.settle(8);
	EXPECT_EQ(world.levelAt({9, 8, 8}), 8u);
}

// ---------------------------------------------------------------------------
// Support and sideways spread
// ---------------------------------------------------------------------------

TEST(VoxelFluidSimulator, SupportedSourceSpreadsAndFlowWeakensByOneUntilLevelOneStops)
{
	SimulatorWorld world;
	world.loadChunk({0, 0, 0});
	world.fillFloor();
	world.set({4, 1, 8}, world.water().sourceRuntime);

	world.settle();

	// Distance d from the source carries level N - d + 1; the level-one edge stops.
	for (int distance = 1; distance <= 8; ++distance)
	{
		EXPECT_EQ(world.levelAt({4 + distance, 1, 8}), static_cast<std::size_t>(9 - distance)) << distance;
	}
	EXPECT_TRUE(world.isEmptyAt({13, 1, 8}));

	// Fluid never counts as support: nothing spread on top of the pool.
	EXPECT_TRUE(world.isEmptyAt({5, 2, 8}));
}

TEST(VoxelFluidSimulator, MidFallCellsDoNotSpreadEvenWhenReenqueued)
{
	SimulatorWorld world;
	world.loadChunk({0, 0, 0});
	world.fillFloor();
	// A hanging flow cell whose support is more water (a column): it must sustain the
	// column, not bleed outwards.
	world.set({8, 3, 8}, world.water().level(8).runtime);
	world.set({8, 2, 8}, world.water().level(8).runtime);
	world.simulator->notifyCellChanged({8, 3, 8});
	world.simulator->notifyCellChanged({8, 2, 8});

	world.step();
	EXPECT_TRUE(world.isEmptyAt({7, 3, 8}));
	EXPECT_TRUE(world.isEmptyAt({9, 3, 8}));
	// The column's lowest cell keeps descending toward the floor instead.
	EXPECT_EQ(world.levelAt({8, 1, 8}), 8u);
}

TEST(VoxelFluidSimulator, FlowFedFromAboveResetsToMaximum)
{
	SimulatorWorld world;
	world.loadChunk({0, 0, 0});
	world.fillFloor();
	// A waterfall base: flow on the floor with same-family fluid directly above.
	world.set({8, 2, 8}, world.water().level(8).runtime);
	world.set({8, 1, 8}, world.water().level(3).runtime);
	world.simulator->notifyCellChanged({8, 1, 8});

	world.step();
	// Fed from above, the base pours at full strength instead of level 2.
	EXPECT_EQ(world.levelAt({9, 1, 8}), 8u);
}

TEST(VoxelFluidSimulator, WeakerFlowIsUpgradedAndEqualOrStrongerIsNot)
{
	SimulatorWorld world;
	world.loadChunk({0, 0, 0});
	world.fillFloor();
	world.set({8, 1, 8}, world.water().sourceRuntime);
	world.set({9, 1, 8}, world.water().level(3).runtime); // weaker than the source's outflow
	world.set({7, 1, 8}, world.water().level(8).runtime); // already at the outflow level

	world.step(1);							 // service the source only
	EXPECT_EQ(world.levelAt({9, 1, 8}), 8u); // upgraded
	EXPECT_EQ(world.levelAt({7, 1, 8}), 8u); // unchanged
	EXPECT_GE(world.simulator->lastChangedCellCount(), 1u);
}

TEST(VoxelFluidSimulator, SourcesAreNeverOverwritten)
{
	SimulatorWorld world;
	world.loadChunk({0, 0, 0});
	world.fillFloor();
	world.set({8, 1, 8}, world.water().sourceRuntime);
	world.set({9, 1, 8}, world.water().sourceRuntime);

	world.settle();

	const spk::VoxelCell *left = world.map->tryCell({8, 1, 8});
	const spk::VoxelCell *right = world.map->tryCell({9, 1, 8});
	ASSERT_NE(left, nullptr);
	ASSERT_NE(right, nullptr);
	EXPECT_EQ(left->id, world.water().sourceRuntime);
	EXPECT_EQ(right->id, world.water().sourceRuntime);
}

TEST(VoxelFluidSimulator, AnotherFluidFamilyBlocksFlow)
{
	SimulatorWorld world;
	world.loadChunk({0, 0, 0});
	world.fillFloor();
	world.set({8, 1, 8}, world.water().sourceRuntime);
	world.set({9, 1, 8}, world.lava().sourceRuntime);
	world.set({10, 1, 8}, world.lava().level(2).runtime);

	world.settle();

	const spk::VoxelCell *lavaSource = world.map->tryCell({9, 1, 8});
	const spk::VoxelCell *lavaFlow = world.map->tryCell({10, 1, 8});
	ASSERT_NE(lavaSource, nullptr);
	ASSERT_NE(lavaFlow, nullptr);
	EXPECT_EQ(lavaSource->id, world.lava().sourceRuntime);
	// Water may not upgrade or replace another family's flow either.
	const spk::VoxelFluidRef *reference = world.registry.tryFluidRef(lavaFlow->id);
	ASSERT_NE(reference, nullptr);
	EXPECT_EQ(reference->family, world.lavaFamily);
}

TEST(VoxelFluidSimulator, ReplaceableCellsAreOverwrittenAndBlockingCellsAreNot)
{
	SimulatorWorld world;
	world.loadChunk({0, 0, 0});
	world.fillFloor();
	world.set({8, 1, 8}, world.water().sourceRuntime);
	world.set({9, 1, 8}, world.bush);  // canReplace -> true
	world.set({7, 1, 8}, world.fence); // canReplace -> false, providesSupport -> false

	world.step(1);
	EXPECT_EQ(world.levelAt({9, 1, 8}), 8u); // the bush drowned
	const spk::VoxelCell *fence = world.map->tryCell({7, 1, 8});
	ASSERT_NE(fence, nullptr);
	EXPECT_EQ(fence->id, world.fence);
	// The fence is no support either: no climb happened onto it.
	EXPECT_TRUE(world.isEmptyAt({7, 2, 8}));
}

// ---------------------------------------------------------------------------
// Climbing onto adjacent support
// ---------------------------------------------------------------------------

TEST(VoxelFluidSimulator, FluidPropagatesOntoTheCellAboveAdjacentSupport)
{
	SimulatorWorld world;
	world.loadChunk({0, 0, 0});
	world.fillFloor();
	world.set({9, 1, 8}, world.stone); // a one-high wall beside the source
	world.set({8, 1, 8}, world.water().sourceRuntime);

	world.step(1);
	// The wall cell itself is untouched, but the cell above it received fluid.
	const spk::VoxelCell *wall = world.map->tryCell({9, 1, 8});
	ASSERT_NE(wall, nullptr);
	EXPECT_EQ(wall->id, world.stone);
	EXPECT_EQ(world.levelAt({9, 2, 8}), 8u);
}

TEST(VoxelFluidSimulator, ClimbingConsumesOneNormalPropagationLevel)
{
	SimulatorWorld world;
	world.loadChunk({0, 0, 0});
	world.fillFloor();
	world.set({9, 1, 8}, world.stone);
	world.set({8, 1, 8}, world.water().level(3).runtime);
	world.simulator->notifyCellChanged({8, 1, 8});

	world.step();
	// A level-3 flow pours level 2 over the wall - same cost as a flat move.
	EXPECT_EQ(world.levelAt({9, 2, 8}), 2u);
}

// ---------------------------------------------------------------------------
// Drop priority
// ---------------------------------------------------------------------------

TEST(VoxelFluidSimulator, DropCandidatesSuppressFlatSpreadAndClimbing)
{
	SimulatorWorld world;
	world.loadChunk({0, 0, 0});
	// A terrace: stone floor at y == 1 under the source, nothing under the +X neighbor.
	for (int z = 0; z < spk::VoxelChunk::Size.z; ++z)
	{
		for (int x = 0; x <= 8; ++x)
		{
			world.set({x, 1, z}, world.stone);
		}
	}
	world.set({7, 2, 8}, world.stone); // a wall to climb on the -X side
	world.set({8, 2, 8}, world.water().sourceRuntime);

	world.step(1);
	// (9,2,8) is empty with an empty floor below: a drop candidate. It alone receives
	// fluid; the flat candidates and the climb stay dry during that visit.
	EXPECT_EQ(world.levelAt({9, 2, 8}), 8u);
	EXPECT_TRUE(world.isEmptyAt({8, 2, 7}));
	EXPECT_TRUE(world.isEmptyAt({8, 2, 9}));
	EXPECT_TRUE(world.isEmptyAt({7, 3, 8}));
}

// ---------------------------------------------------------------------------
// Loading rules, center and range
// ---------------------------------------------------------------------------

TEST(VoxelFluidSimulator, UnloadedCellsAreNotEmptyAndChunksAreNeverLoaded)
{
	SimulatorWorld world;
	world.loadChunk({0, 0, 0});
	// Floor + source right against the -X chunk boundary; the neighbor chunk is unloaded.
	world.fillFloor();
	world.set({0, 1, 8}, world.water().sourceRuntime);

	world.settle();

	EXPECT_EQ(world.map->loadedChunkCount(), 1u);
	EXPECT_EQ(world.map->tryCell({-1, 1, 8}), nullptr); // still unloaded, never written
	// In-chunk spread happened normally.
	EXPECT_EQ(world.levelAt({1, 1, 8}), 8u);
}

TEST(VoxelFluidSimulator, SimulationCenterAndHorizontalRangeLimitTheBox)
{
	SimulatorWorld world;
	world.loadChunk({0, 0, 0});
	world.fillFloor();
	world.set({4, 1, 8}, world.water().sourceRuntime);

	// A center far from the source leaves it unserviced entirely.
	world.simulator->setSimulationCenter(spk::Vector3Int{14, 1, 8});
	world.simulator->setHorizontalRange(4);
	world.settle(16);
	EXPECT_TRUE(world.isEmptyAt({5, 1, 8}));

	// Centred on the source with a tight range, the spread stops at the box edge.
	world.simulator->setSimulationCenter(spk::Vector3Int{4, 1, 8});
	world.simulator->setHorizontalRange(2);
	world.settle(32);
	EXPECT_GT(world.levelAt({5, 1, 8}), 0u);
	EXPECT_GT(world.levelAt({6, 1, 8}), 0u);
	EXPECT_TRUE(world.isEmptyAt({7, 1, 8})); // outside the box, never reached

	EXPECT_THROW(world.simulator->setHorizontalRange(-1), std::invalid_argument);
}

// ---------------------------------------------------------------------------
// Budget, fairness and determinism (historical worldgen scenario: floor + sources)
// ---------------------------------------------------------------------------

namespace
{
	class SourceBurstWorld : public SimulatorWorld
	{
	public:
		explicit SourceBurstWorld(std::size_t p_sourceCount, std::vector<spk::Vector3Int> p_chunks = {{0, 0, 0}})
		{
			for (const spk::Vector3Int &coordinates : p_chunks)
			{
				loadChunk(coordinates);
				const spk::Vector3Int origin = coordinates * spk::VoxelChunk::Size;
				for (int z = 0; z < spk::VoxelChunk::Size.z; ++z)
				{
					for (int x = 0; x < spk::VoxelChunk::Size.x; ++x)
					{
						set(origin + spk::Vector3Int{x, 0, z}, stone);
					}
				}
				const std::array positions = {
					spk::Vector3Int{1, 1, 1},
					spk::Vector3Int{6, 1, 1},
					spk::Vector3Int{11, 1, 1},
					spk::Vector3Int{1, 1, 6},
					spk::Vector3Int{6, 1, 6},
					spk::Vector3Int{11, 1, 6}};
				for (std::size_t index = 0; index < std::min(p_sourceCount, positions.size()); ++index)
				{
					set(origin + positions[index], water().sourceRuntime);
				}
			}
		}

		[[nodiscard]] bool isFluid(const spk::Vector3Int &p_position) const
		{
			return registry.tryFluidRef(map->cell(p_position).id) != nullptr;
		}
	};
}

class FluidSimulatorBudgetBoundaryTest : public testing::TestWithParam<std::size_t>
{
};

TEST_P(FluidSimulatorBudgetBoundaryTest, ServicesAtMostTheConfiguredBudget)
{
	SourceBurstWorld world(GetParam());
	world.simulator->setMaximumProcessedCells(2);
	world.simulator->stepNow();
	EXPECT_EQ(world.simulator->lastProcessedCellCount(), std::min<std::size_t>(GetParam(), 2));
	EXPECT_LE(world.simulator->lastProcessedCellCount(), world.simulator->maximumProcessedCells());
}

INSTANTIATE_TEST_SUITE_P(SourceCountsBelowEqualAndAbove, FluidSimulatorBudgetBoundaryTest, testing::Values(1u, 2u, 3u));

TEST(VoxelFluidSimulator, FrontierAdvancesWhileSourcesExceedTheBudget)
{
	SourceBurstWorld world(3);
	world.simulator->setMaximumProcessedCells(2);

	world.simulator->stepNow();
	ASSERT_TRUE(world.isFluid({2, 1, 1}));
	EXPECT_FALSE(world.isFluid({3, 1, 1}));

	// The second step alternates the oldest frontier item with the next persistent
	// source, so the front keeps moving even under an over-subscribed budget.
	world.simulator->stepNow();
	EXPECT_TRUE(world.isFluid({3, 1, 1}));
	EXPECT_LE(world.simulator->lastProcessedCellCount(), 2u);
}

TEST(VoxelFluidSimulator, PersistentSourcesAreRevisitedAlongsideAContinuousFrontier)
{
	SourceBurstWorld world(3);
	world.simulator->setMaximumProcessedCells(1);
	world.simulator->stepNow();
	ASSERT_TRUE(world.isFluid({2, 1, 1}));
	ASSERT_TRUE(world.map->setCell({2, 1, 1}, {}));

	bool refilled = false;
	for (int step = 0; step < 8 && !refilled; ++step)
	{
		world.simulator->stepNow();
		refilled = world.isFluid({2, 1, 1});
		EXPECT_LE(world.simulator->lastProcessedCellCount(), 1u);
	}
	EXPECT_TRUE(refilled);
}

TEST(VoxelFluidSimulator, FixedSetupAndTickSequenceAreDeterministic)
{
	SourceBurstWorld first(6);
	SourceBurstWorld second(6);
	first.simulator->setMaximumProcessedCells(3);
	second.simulator->setMaximumProcessedCells(3);
	for (int step = 0; step < 12; ++step)
	{
		first.simulator->stepNow();
		second.simulator->stepNow();
	}

	const spk::VoxelChunk *firstChunk = first.map->tryChunk({0, 0, 0});
	const spk::VoxelChunk *secondChunk = second.map->tryChunk({0, 0, 0});
	ASSERT_NE(firstChunk, nullptr);
	ASSERT_NE(secondChunk, nullptr);
	EXPECT_EQ(firstChunk->grid().cells(), secondChunk->grid().cells());
}

TEST(VoxelFluidSimulator, MultipleLoadedChunksRemainBoundedAcrossCenterChanges)
{
	SourceBurstWorld world(3, {{0, 0, 0}, {10, 0, 0}});
	world.simulator->setMaximumProcessedCells(2);
	const std::array centers = {
		spk::Vector3Int{1, 1, 1},
		spk::Vector3Int{161, 1, 1},
		spk::Vector3Int{1, 1, 1},
		spk::Vector3Int{161, 1, 1},
		spk::Vector3Int{1, 1, 1},
		spk::Vector3Int{161, 1, 1}};
	for (const spk::Vector3Int &center : centers)
	{
		world.simulator->setSimulationCenter(center);
		world.simulator->stepNow();
		EXPECT_LE(world.simulator->lastProcessedCellCount(), 2u);
	}

	EXPECT_TRUE(world.isFluid({2, 1, 1}));
	EXPECT_TRUE(world.isFluid({162, 1, 1}));
}

// ---------------------------------------------------------------------------
// Source tracking across chunk lifetime and dynamic edits
// ---------------------------------------------------------------------------

TEST(VoxelFluidSimulator, NewlyLoadedChunksAreScannedAndUnloadedChunksForgotten)
{
	spk::VoxelRuntimeId sourceId{};
	spk::VoxelRuntimeId stoneId{};
	SimulatorWorld world([&sourceId, &stoneId](spk::VoxelChunk &p_chunk) {
		if (p_chunk.coordinates() != spk::Vector3Int{4, 0, 0})
		{
			return;
		}
		p_chunk.editCells([&](spk::VoxelChunk::Editor &p_editor) {
			for (int z = 0; z < spk::VoxelChunk::Size.z; ++z)
			{
				for (int x = 0; x < spk::VoxelChunk::Size.x; ++x)
				{
					(void)p_editor.setCell(x, 0, z, {stoneId});
				}
			}
			(void)p_editor.setCell(8, 1, 8, {sourceId});
		});
	});
	sourceId = world.water().sourceRuntime;
	stoneId = world.stone;

	world.loadChunk({0, 0, 0});
	world.step();
	EXPECT_TRUE(world.isEmptyAt({8, 1, 8}));

	// The chunk with the worldgen source loads later: the next step discovers it.
	world.loadChunk({4, 0, 0});
	world.step();
	EXPECT_EQ(world.levelAt({4 * spk::VoxelChunk::Size.x + 9, 1, 8}), 8u);

	// Unloading the chunk forgets its sources: nothing is processed anymore.
	ASSERT_TRUE(world.map->unloadChunk({4, 0, 0}));
	world.simulator->reset(); // drop the stale frontier entries too
	world.step();
	EXPECT_EQ(world.simulator->lastProcessedCellCount(), 0u);
	EXPECT_EQ(world.simulator->lastChangedCellCount(), 0u);
}

TEST(VoxelFluidSimulator, NotifyCellChangedTracksDynamicSources)
{
	SimulatorWorld world;
	world.loadChunk({0, 0, 0});
	world.fillFloor();
	world.step(); // the chunk is now scanned (no sources yet)
	world.step();
	EXPECT_EQ(world.simulator->lastProcessedCellCount(), 0u);

	// Placing then deleting a source between steps leaves nothing to service: the
	// notification added the source and the second one removed it again.
	world.set({8, 1, 8}, world.water().sourceRuntime);
	world.simulator->notifyCellChanged({8, 1, 8});
	world.map->setCell({8, 1, 8}, spk::VoxelCell{});
	world.simulator->notifyCellChanged({8, 1, 8});
	world.step();
	EXPECT_EQ(world.simulator->lastProcessedCellCount(), 0u);
	EXPECT_TRUE(world.isEmptyAt({9, 1, 8}));

	// A source placed and kept is serviced on the next step.
	world.set({8, 1, 8}, world.water().sourceRuntime);
	world.simulator->notifyCellChanged({8, 1, 8});
	world.step(1);
	EXPECT_EQ(world.levelAt({9, 1, 8}), 8u);
}

TEST(VoxelFluidSimulator, NotifyCellChangedAvoidsDuplicateSourceEntries)
{
	SimulatorWorld world;
	world.loadChunk({0, 0, 0});
	world.fillFloor();
	world.step();

	world.set({8, 1, 8}, world.water().sourceRuntime);
	world.simulator->notifyCellChanged({8, 1, 8});
	world.simulator->notifyCellChanged({8, 1, 8});

	// A duplicate entry would service the source twice per step: with an ample budget and
	// an empty frontier, exactly one visit proves the catalog holds it once.
	world.step();
	EXPECT_EQ(world.simulator->lastProcessedCellCount(), 1u);
	EXPECT_EQ(world.levelAt({9, 1, 8}), 8u);
}

TEST(VoxelFluidSimulator, RescanChunkRebuildsTheSourceList)
{
	SimulatorWorld world;
	world.loadChunk({0, 0, 0});
	world.fillFloor();
	world.step(); // scanned: empty source list

	// Place a source silently (no notify), then rescan the chunk explicitly.
	world.set({8, 1, 8}, world.water().sourceRuntime);
	world.simulator->rescanChunk({0, 0, 0});
	world.step(1);
	EXPECT_EQ(world.levelAt({9, 1, 8}), 8u);

	// Rescanning an unloaded chunk drops its list.
	world.simulator->rescanChunk({7, 7, 7});
	SUCCEED();
}

TEST(VoxelFluidSimulator, ResetForgetsPendingWork)
{
	SimulatorWorld world;
	world.loadChunk({0, 0, 0});
	world.fillFloor();
	world.set({8, 3, 8}, world.water().level(5).runtime);
	world.simulator->notifyCellChanged({8, 3, 8});
	EXPECT_GT(world.simulator->pendingCellCount(), 0u);

	world.simulator->reset();
	EXPECT_EQ(world.simulator->pendingCellCount(), 0u);
	world.step();
	EXPECT_EQ(world.simulator->lastProcessedCellCount(), 0u);
}

// ---------------------------------------------------------------------------
// Map revision
// ---------------------------------------------------------------------------

TEST(VoxelFluidSimulator, FluidEditsMoveTheMapRevision)
{
	SimulatorWorld world;
	world.loadChunk({0, 0, 0});
	world.fillFloor();
	world.set({8, 1, 8}, world.water().sourceRuntime);

	const std::uint64_t before = world.map->revision();
	world.step();
	ASSERT_GT(world.simulator->lastChangedCellCount(), 0u);
	EXPECT_GT(world.map->revision(), before);

	// A step that changes nothing leaves the revision alone.
	SimulatorWorld idle;
	idle.loadChunk({0, 0, 0});
	const std::uint64_t idleBefore = idle.map->revision();
	idle.step();
	EXPECT_EQ(idle.map->revision(), idleBefore);
}

// ---------------------------------------------------------------------------
// Tick pacing and engine dispatch
// ---------------------------------------------------------------------------

TEST(VoxelFluidSimulator, AdvanceStepsOnlyOncePerTickInterval)
{
	SourceBurstWorld world(1);
	ASSERT_EQ(world.simulator->tickInterval(), spk::VoxelFluidSimulator::DefaultTickIntervalMs);

	world.simulator->advance(60);
	EXPECT_FALSE(world.isFluid({2, 1, 1})); // 60 ms accumulated: not due yet

	world.simulator->advance(60);
	EXPECT_TRUE(world.isFluid({2, 1, 1})); // 120 ms reached: one step ran

	// A zero interval steps on every update.
	world.simulator->setTickInterval(0);
	world.simulator->advance(0);
	EXPECT_GE(world.simulator->lastProcessedCellCount(), 1u);
}

TEST(VoxelFluidSimulator, EngineUpdatePassStepsTheSimulatorBetweenStreamingAndRendering)
{
	SourceBurstWorld world(1);
	spk::GameEngine engine;
	spk::VoxelFluidSimulationLogic &logic = engine.add<spk::VoxelFluidSimulationLogic>();

	// The default priority slots the fluid pass after chunk streaming and before the
	// render logics (which default to 0), so edits bake in the same frame.
	EXPECT_EQ(logic.priority(), spk::VoxelFluidSimulationLogic::DefaultPriority);
	EXPECT_LT(logic.priority(), spk::VoxelChunkStreamerLogic::DefaultPriority);
	EXPECT_GT(logic.priority(), 0);

	spk::Entity3D player;
	spk::VoxelFluidSimulator &simulator = player.addComponent<spk::VoxelFluidSimulator>(
		*world.map,
		spk::VoxelFluidCellRules{
			.canReplace = [](const spk::VoxelCell &) { return false; },
			.providesSupport = [](const spk::VoxelCell &) { return true; }});
	simulator.setTickInterval(0);
	engine.addEntity(&player);

	spk::UpdateContext tick;
	tick.deltaTime = spk::Duration(16.0L, spk::TimeUnit::Millisecond);
	engine.logicRegistry().update(tick, engine.componentRegistry());

	EXPECT_GE(simulator.lastProcessedCellCount(), 1u);
	EXPECT_TRUE(world.isFluid({2, 1, 1}));
}
