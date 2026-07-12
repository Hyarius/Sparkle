#include <gtest/gtest.h>

#include <array>
#include <cstdint>
#include <iostream>
#include <memory>

#include "structures/game_engine/spk_camera_3d.hpp"
#include "structures/game_engine/spk_game_engine.hpp"
#include "structures/graphics/rendering/unit/spk_render_unit_builder.hpp"
#include "structures/graphics/spk_texture.hpp"
#include "structures/math/spk_perlin.hpp"
#include "structures/system/time/spk_chronometer.hpp"
#include "structures/voxel/spk_cross_plane_voxel_shape.hpp"
#include "structures/voxel/spk_cube_voxel_shape.hpp"
#include "structures/voxel/spk_slab_voxel_shape.hpp"
#include "structures/voxel/spk_slope_voxel_shape.hpp"
#include "structures/voxel/spk_stair_voxel_shape.hpp"
#include "structures/voxel/spk_voxel_chunk_render_logic.hpp"
#include "structures/voxel/spk_voxel_map.hpp"

namespace
{
	// Provisional budget for baking the meshes and assembling the draw commands of a
	// freshly streamed 8x8 chunk square. To be tightened once the target value is agreed.
	constexpr long long MaximumRenderPassMilliseconds = 2000;
	constexpr long long MaximumGenerationMilliseconds = 2000;

	constexpr int ChunkSpan = 8;

	[[nodiscard]] int positiveModulo(int p_value, int p_modulus)
	{
		const int result = p_value % p_modulus;
		return result < 0 ? result + p_modulus : result;
	}

	class InstrumentedVoxelChunkRenderLogic : public spk::VoxelChunkRenderLogic
	{
	public:
		using spk::VoxelChunkRenderLogic::_executeRender;
		using spk::VoxelChunkRenderLogic::_executeUpdate;
		using spk::VoxelChunkRenderLogic::_onRenderStarted;
		using spk::VoxelChunkRenderLogic::_onUpdateStarted;
		using spk::VoxelChunkRenderLogic::_parseComponentForRender;
		using spk::VoxelChunkRenderLogic::_parseComponentForUpdate;
		using spk::VoxelChunkRenderLogic::VoxelChunkRenderLogic;
	};
}

TEST(VoxelRenderPerformance, BakesAnEightByEightChunkSquareWithinBudget)
{
	spk::VoxelRegistry registry;
	const spk::VoxelRuntimeId cube = registry.registerShape(std::make_unique<spk::CubeVoxelShape>(spk::AtlasCell{0, 0}));
	const spk::VoxelRuntimeId slope = registry.registerShape(std::make_unique<spk::SlopeVoxelShape>(spk::AtlasCell{1, 0}));
	const spk::VoxelRuntimeId slab = registry.registerShape(std::make_unique<spk::SlabVoxelShape>(spk::AtlasCell{1, 0}));
	const spk::VoxelRuntimeId stair = registry.registerShape(std::make_unique<spk::StairVoxelShape>(spk::AtlasCell{2, 0}, 4));
	const spk::VoxelRuntimeId crossPlane = registry.registerShape(std::make_unique<spk::DiagonalCrossVoxelShape>(spk::AtlasCell{3, 0}));

	const spk::Perlin perlin(spk::Perlin::Parameters{.seed = 12345, .octaves = 4, .persistence = 0.5f, .lacunarity = 2.0f, .frequency = 1.0f / 32.0f});

	spk::GameEngine engine;
	std::size_t generatedVoxelCount = 0;
	spk::VoxelMap map(
		registry,
		[&](spk::VoxelChunk &p_chunk) {
			const spk::Vector3Int origin = p_chunk.worldOrigin();
			constexpr std::array orientations = {
				spk::VoxelOrientation::PositiveX,
				spk::VoxelOrientation::PositiveZ,
				spk::VoxelOrientation::NegativeX,
				spk::VoxelOrientation::NegativeZ};
			p_chunk.editCells([&](spk::VoxelChunk::Editor &p_editor) {
				for (int z = 0; z < spk::VoxelChunk::Size.z; ++z)
				{
					for (int x = 0; x < spk::VoxelChunk::Size.x; ++x)
					{
						const int worldX = origin.x + x;
						const int worldZ = origin.z + z;
						const float rawHeight = perlin.sample2D(
							static_cast<float>(worldX),
							static_cast<float>(worldZ),
							1.0f,
							static_cast<float>(spk::VoxelChunk::Size.y - 1));

						const int height = std::clamp(
							static_cast<int>(rawHeight),
							1,
							spk::VoxelChunk::Size.y);

						for (int y = 0; y < height - 1; ++y)
						{
							const bool carveCave = y > 0 && positiveModulo(worldX * 31 + y * 17 + worldZ * 13, 23) == 0;
							if (carveCave == false)
							{
								(void)p_editor.setCell(x, y, z, {cube});
								++generatedVoxelCount;
							}
						}

						const int topY = height - 1;
						const int topPattern = positiveModulo(worldX * 7 + worldZ * 11, 8);
						const spk::VoxelOrientation orientation = orientations[static_cast<std::size_t>(positiveModulo(worldX + worldZ, 4))];
						const spk::VoxelFlip flip = positiveModulo(worldX * 3 + worldZ, 2) == 0
														? spk::VoxelFlip::PositiveY
														: spk::VoxelFlip::NegativeY;
						const spk::VoxelRuntimeId topShape = topPattern < 2 ? slab : topPattern < 4 ? slope
																		  : topPattern < 6	 ? stair
																							 : cube;
						(void)p_editor.setCell(x, topY, z, {topShape, orientation, flip});
						++generatedVoxelCount;

						// Sparse geometry above the terrain forces the mesher through many exposed,
						// non-cubic faces instead of mostly processing a solid height map.
						for (int y = height; y < spk::VoxelChunk::Size.y; ++y)
						{
							const int structurePattern = positiveModulo(worldX * 19 + y * 7 + worldZ * 29, 13);
							if (structurePattern > 1)
							{
								continue;
							}
							const spk::VoxelRuntimeId shape = structurePattern == 0 ? crossPlane : stair;
							(void)p_editor.setCell(x, y, z, {shape, orientations[static_cast<std::size_t>(positiveModulo(worldX + y + worldZ, 4))], flip});
							++generatedVoxelCount;
						}
					}
				}
			});
		},
		&engine);

	spk::Camera3D camera;
	camera.makeMain();
	spk::Texture texture;
	InstrumentedVoxelChunkRenderLogic logic(texture, false);

	spk::Chronometer generationChronometer;
	generationChronometer.start();
	for (int chunkZ = -ChunkSpan / 2; chunkZ < ChunkSpan / 2; ++chunkZ)
	{
		for (int chunkX = -ChunkSpan / 2; chunkX < ChunkSpan / 2; ++chunkX)
		{
			(void)map.chunk({chunkX, 0, chunkZ});
		}
	}
	const spk::Duration generationElapsed = generationChronometer.elapsedTime();
	ASSERT_EQ(map.loadedChunkCount(), static_cast<std::size_t>(ChunkSpan * ChunkSpan));

	const auto &renderers = engine.componentRegistry().components<spk::VoxelChunkRenderer>();
	std::size_t triangleCount = 0;

	spk::Chronometer chronometer;
	chronometer.start();

	const spk::UpdateContext tick{};
	logic._onUpdateStarted(tick);
	for (spk::Component *component : renderers)
	{
		if (component != nullptr)
		{
			spk::VoxelChunkRenderer &renderer = *static_cast<spk::VoxelChunkRenderer *>(component);
			logic._parseComponentForUpdate(tick, renderer);
		}
	}
	logic._executeUpdate(tick);
	for (spk::Component *component : renderers)
	{
		if (component != nullptr)
		{
			const spk::VoxelRenderMeshes &meshes = static_cast<spk::VoxelChunkRenderer *>(component)->meshes();
			triangleCount += (meshes.opaque.indexes().size() + meshes.transparent.indexes().size()) / 3;
		}
	}

	logic._onRenderStarted(map.loadedChunkCount());
	for (spk::Component *component : renderers)
	{
		if (component != nullptr)
		{
			logic._parseComponentForRender(*static_cast<spk::VoxelChunkRenderer *>(component));
		}
	}
	spk::RenderUnitBuilder builder;
	logic._executeRender(builder);

	const spk::Duration elapsed = chronometer.elapsedTime();

	EXPECT_EQ(builder.size(), static_cast<std::size_t>(ChunkSpan * ChunkSpan));
	EXPECT_FALSE(builder.empty());

	std::cout << "[ PERF     ] 8x8 chunk render pass: " << elapsed.milliseconds() << " ms ("
			  << generatedVoxelCount << " voxels, " << triangleCount << " triangles in " << builder.size() << " meshes)" << std::endl;
	std::cout << "[ PERF     ] 8x8 chunk generation: " << generationElapsed.milliseconds() << " ms" << std::endl;

	EXPECT_LT(generationElapsed.milliseconds(), MaximumGenerationMilliseconds);
	EXPECT_LT(elapsed.milliseconds(), MaximumRenderPassMilliseconds);
	EXPECT_EQ(generatedVoxelCount, 141704u);
	// Exact boundary subtraction retains visible partial faces and may split one source
	// polygon into several remnants; exposed slope/stair inner faces are retained too.
	EXPECT_EQ(triangleCount, 785699u);
}
