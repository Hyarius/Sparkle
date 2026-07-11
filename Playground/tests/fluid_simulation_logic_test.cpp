#include <gtest/gtest.h>

#include "logics/fluid_simulation_logic.hpp"
#include "voxel/voxel_registry.hpp"
#include "world/chunk_provider.hpp"
#include "world/voxel_world.hpp"

#include "structures/voxel/spk_voxel_chunk.hpp"

#include <algorithm>
#include <array>
#include <filesystem>
#include <memory>
#include <vector>

namespace
{
	class FluidChunkProvider final : public pg::IChunkProvider
	{
	private:
		std::int32_t _floor = -1;
		std::int32_t _source = -1;
		std::size_t _sourceCount = 0;

	public:
		FluidChunkProvider(std::int32_t p_floor, std::int32_t p_source, std::size_t p_sourceCount) :
			_floor(p_floor),
			_source(p_source),
			_sourceCount(p_sourceCount)
		{
		}

		void fill(spk::VoxelChunk &p_chunk) const override
		{
			p_chunk.editCells([&](spk::VoxelChunk::Editor &p_editor) {
				for (int z = 0; z < spk::VoxelChunk::Size.z; ++z)
				{
					for (int x = 0; x < spk::VoxelChunk::Size.x; ++x)
					{
						(void)p_editor.setCell(x, 0, z, {_floor});
					}
				}
				const std::array positions = {
					spk::Vector3Int{1, 1, 1},
					spk::Vector3Int{6, 1, 1},
					spk::Vector3Int{11, 1, 1},
					spk::Vector3Int{1, 1, 6},
					spk::Vector3Int{6, 1, 6},
					spk::Vector3Int{11, 1, 6}};
				for (std::size_t index = 0; index < std::min(_sourceCount, positions.size()); ++index)
				{
					(void)p_editor.setCell(positions[index], {_source});
				}
			});
		}
	};

	class FluidSimulation
	{
	public:
		pg::VoxelRegistry registry;
		std::unique_ptr<FluidChunkProvider> provider;
		std::unique_ptr<pg::VoxelWorld> world;
		std::unique_ptr<pg::FluidSimulationLogic> logic;

		explicit FluidSimulation(
			std::size_t p_sourceCount,
			std::vector<pg::ChunkCoordinates> p_chunks = {pg::ChunkCoordinates{{0, 0, 0}}})
		{
			registry.load(std::filesystem::path(PG_RESOURCE_DIR) / "data" / "voxels");
			provider = std::make_unique<FluidChunkProvider>(
				registry.numericId("stone-block"),
				registry.numericId("water"),
				p_sourceCount);
			world = std::make_unique<pg::VoxelWorld>(registry);
			world->setProvider(provider.get());
			for (const pg::ChunkCoordinates &coordinates : p_chunks)
			{
				world->loadChunk(coordinates);
			}
			logic = std::make_unique<pg::FluidSimulationLogic>(*world);
		}
	};

	[[nodiscard]] bool isFluid(const FluidSimulation &p_test, const spk::Vector3Int &p_position)
	{
		return p_test.registry.tryFluidRef(p_test.world->cell(p_position).id) != nullptr;
	}
}

class FluidBudgetBoundaryTest : public testing::TestWithParam<std::size_t>
{
};

TEST_P(FluidBudgetBoundaryTest, ServicesAtMostTheConfiguredBudget)
{
	FluidSimulation test(GetParam());
	test.logic->setMaxCellsPerTick(2);
	test.logic->stepNow();
	EXPECT_EQ(test.logic->lastProcessedCellCount(), std::min<std::size_t>(GetParam(), 2));
	EXPECT_LE(test.logic->lastProcessedCellCount(), test.logic->maxCellsPerTick());
}

INSTANTIATE_TEST_SUITE_P(SourceCountsBelowEqualAndAbove, FluidBudgetBoundaryTest, testing::Values(1u, 2u, 3u));

TEST(FluidSimulationLogic, FrontierAdvancesWhileSourcesExceedTheBudget)
{
	FluidSimulation test(3);
	test.logic->setMaxCellsPerTick(2);

	test.logic->stepNow();
	ASSERT_TRUE(isFluid(test, {2, 1, 1}));
	EXPECT_FALSE(isFluid(test, {3, 1, 1}));

	// The second step alternates the oldest frontier item with the next persistent
	// source. Under the previous source-prefix scheduler this frontier never ran.
	test.logic->stepNow();
	EXPECT_TRUE(isFluid(test, {3, 1, 1}));
	EXPECT_LE(test.logic->lastProcessedCellCount(), 2u);
}

TEST(FluidSimulationLogic, PersistentSourcesAreRevisitedAlongsideAContinuousFrontier)
{
	FluidSimulation test(3);
	test.logic->setMaxCellsPerTick(1);
	test.logic->stepNow();
	ASSERT_TRUE(isFluid(test, {2, 1, 1}));
	ASSERT_TRUE(test.world->setCell({2, 1, 1}, {}));

	bool refilled = false;
	for (int step = 0; step < 8 && !refilled; ++step)
	{
		test.logic->stepNow();
		refilled = isFluid(test, {2, 1, 1});
		EXPECT_LE(test.logic->lastProcessedCellCount(), 1u);
	}
	EXPECT_TRUE(refilled);
}

TEST(FluidSimulationLogic, FixedSetupAndTickSequenceAreDeterministic)
{
	FluidSimulation first(6);
	FluidSimulation second(6);
	first.logic->setMaxCellsPerTick(3);
	second.logic->setMaxCellsPerTick(3);
	for (int step = 0; step < 12; ++step)
	{
		first.logic->stepNow();
		second.logic->stepNow();
	}

	const spk::VoxelChunk *firstChunk = first.world->map().tryChunk({0, 0, 0});
	const spk::VoxelChunk *secondChunk = second.world->map().tryChunk({0, 0, 0});
	ASSERT_NE(firstChunk, nullptr);
	ASSERT_NE(secondChunk, nullptr);
	EXPECT_EQ(firstChunk->grid().cells(), secondChunk->grid().cells());
}

TEST(FluidSimulationLogic, MultipleLoadedChunksRemainBoundedAcrossRangeChanges)
{
	FluidSimulation test(
		3,
		{pg::ChunkCoordinates{{0, 0, 0}}, pg::ChunkCoordinates{{10, 0, 0}}});
	test.logic->setMaxCellsPerTick(2);
	const std::array centers = {
		spk::Vector3Int{1, 1, 1},
		spk::Vector3Int{161, 1, 1},
		spk::Vector3Int{1, 1, 1},
		spk::Vector3Int{161, 1, 1},
		spk::Vector3Int{1, 1, 1},
		spk::Vector3Int{161, 1, 1}};
	for (const spk::Vector3Int &center : centers)
	{
		test.logic->setSimulationCenter(center);
		test.logic->stepNow();
		EXPECT_LE(test.logic->lastProcessedCellCount(), 2u);
	}

	EXPECT_TRUE(isFluid(test, {2, 1, 1}));
	EXPECT_TRUE(isFluid(test, {162, 1, 1}));
}
