#include <gtest/gtest.h>

#include "voxel/voxel_registry.hpp"
#include "world/voxel_world.hpp"

#include "structures/voxel/spk_voxel_chunk.hpp"
#include "structures/voxel/spk_voxel_fluid_simulator.hpp"

#include <cstdint>
#include <filesystem>
#include <memory>

// The fluid engine (registration, propagation, scheduling) lives in Sparkle and is tested
// there; what Playground still owns is content and wiring. These tests instantiate
// spk::VoxelFluidSimulator exactly like GameSceneWidget does - shipped water.json,
// traversal-based cell rules - and prove that combination flows.
namespace
{
	class FluidWorld
	{
	public:
		pg::VoxelRegistry registry;
		std::unique_ptr<pg::VoxelWorld> world;
		std::unique_ptr<spk::VoxelFluidSimulator> simulator;

		FluidWorld()
		{
			pg::ShapeCatalog shapes;
			spk::loadJsonDirectory(
				shapes,
				std::filesystem::path(PG_RESOURCE_DIR) / "data" / "shapes",
				[](std::string_view p_id, pg::JsonReader &p_reader) {
					pg::ShapeDefinition definition = pg::parseShapeDefinition(p_reader);
					definition.id = p_id;
					return definition;
				});
			registry.load(shapes, std::filesystem::path(PG_RESOURCE_DIR) / "data" / "voxels");

			const spk::VoxelRuntimeId floor = registry.numericId("stone-block");
			const spk::VoxelRuntimeId source = registry.numericId("water");
			world = std::make_unique<pg::VoxelWorld>(registry, [floor, source](spk::VoxelChunk &p_chunk) {
				p_chunk.editCells([&](spk::VoxelChunk::Editor &p_editor) {
					for (int z = 0; z < spk::VoxelChunk::Size.z; ++z)
					{
						for (int x = 0; x < spk::VoxelChunk::Size.x; ++x)
						{
							(void)p_editor.setCell(x, 0, z, {floor});
						}
					}
					(void)p_editor.setCell({8, 1, 8}, {source});
				});
			});
			world->loadChunk({0, 0, 0});

			// The exact rules GameSceneWidget hands the simulator: passable voxels are
			// replaceable by fluid, solid voxels are support.
			simulator = std::make_unique<spk::VoxelFluidSimulator>(
				world->map(),
				spk::VoxelFluidCellRules{
					.canReplace =
						[this](const spk::VoxelCell &p_cell) {
							return registry.definition(p_cell.id).data.traversal == pg::VoxelTraversal::Passable;
						},
					.providesSupport =
						[this](const spk::VoxelCell &p_cell) {
							return registry.definition(p_cell.id).data.traversal == pg::VoxelTraversal::Solid;
						}});
		}

		[[nodiscard]] bool isFluid(const spk::Vector3Int &p_position) const
		{
			return registry.renderRegistry().tryFluidRef(world->cell(p_position).id) != nullptr;
		}
	};
}

TEST(FluidIntegration, ShippedWaterFlowsUnderTheTraversalRules)
{
	FluidWorld test;
	for (int step = 0; step < 12; ++step)
	{
		test.simulator->stepNow();
	}

	// The worldgen-style source spread across the stone floor and weakened with distance
	// (water.json ships maxSpread 8: the pool ends eight cells out).
	EXPECT_TRUE(test.isFluid({9, 1, 8}));
	EXPECT_TRUE(test.isFluid({12, 1, 8}));
	EXPECT_FALSE(test.isFluid({8, 2, 8})); // water is passable, never support: no stacking
}

TEST(FluidIntegration, FluidEditsMoveTheWorldRevision)
{
	FluidWorld test;
	const std::size_t before = test.world->revision();
	test.simulator->stepNow();

	// The simulation writes through the map, whose revision now backs the world revision
	// the navigation graph watches - engine-side edits invalidate it like player edits.
	EXPECT_GT(test.world->revision(), before);
	EXPECT_EQ(test.world->revision(), static_cast<std::size_t>(test.world->map().revision()));
}
