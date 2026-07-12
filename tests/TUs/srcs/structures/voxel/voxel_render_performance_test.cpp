#include <gtest/gtest.h>

#include <algorithm>
#include <array>
#include <cstdint>
#include <iostream>
#include <memory>

#include "structures/game_engine/spk_camera_3d.hpp"
#include "structures/game_engine/spk_game_engine.hpp"
#include "structures/game_engine/rendering/spk_scene_render_passes.hpp"
#include "structures/graphics/rendering/pass/spk_render_pass_bucket_pack.hpp"
#include "structures/graphics/rendering/unit/spk_render_unit_builder.hpp"
#include "structures/graphics/spk_texture.hpp"
#include "structures/math/spk_perlin.hpp"
#include "structures/system/spk_profiler.hpp"
#include "structures/system/time/spk_chronometer.hpp"
#include "structures/voxel/spk_cross_plane_voxel_shape.hpp"
#include "structures/voxel/spk_cube_voxel_shape.hpp"
#include "structures/voxel/spk_slab_voxel_shape.hpp"
#include "structures/voxel/spk_slope_voxel_shape.hpp"
#include "structures/voxel/spk_stair_voxel_shape.hpp"
#include "structures/voxel/spk_voxel_chunk.hpp"
#include "structures/voxel/spk_voxel_chunk_render_logic.hpp"
#include "structures/voxel/spk_voxel_map.hpp"
#include "structures/voxel/spk_voxel_mesher.hpp"

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
																				 : topPattern < 6	? stair
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
	InstrumentedVoxelChunkRenderLogic logic(texture);

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

	spk::Profiler profiler;
	const spk::UpdateContext tick{.profiler = &profiler};
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

	spk::RenderPassBucketPack passes;
	spk::RenderFrameBuildContext frame{.passes = passes};
	const spk::RenderPass::ScopeId scope{1};
	const spk::Viewport viewport(spk::Rect2D(0, 0, 1, 1));
	const spk::SceneRenderFrameRequest request{.mainTarget = {.frameBuffer = nullptr, .viewport = viewport}, .mainClear = {}};
	auto &opaquePass = passes.emplacePass<spk::RenderPass>(
		{.type = spk::SceneRenderPasses::MainOpaque, .scope = scope}, 1000, "MainOpaque",
		{.debugName = "MainOpaque", .target = request.mainTarget, .clear = {}});
	const spk::SceneRenderBuildContext renderContext{.frame = frame, .sceneScope = scope, .request = request, .components = engine.componentRegistry(), .mainCamera = &camera};
	logic._onRenderStarted(renderContext, map.loadedChunkCount());
	for (spk::Component *component : renderers)
	{
		if (component != nullptr)
		{
			logic._parseComponentForRender(renderContext, *static_cast<spk::VoxelChunkRenderer *>(component));
		}
	}
	logic._executeRender(renderContext);

	const spk::Duration elapsed = chronometer.elapsedTime();

	EXPECT_EQ(opaquePass.commandCount(), static_cast<std::size_t>(ChunkSpan * ChunkSpan));

	EXPECT_LT(generationElapsed.milliseconds(), MaximumGenerationMilliseconds);
	EXPECT_LT(elapsed.milliseconds(), MaximumRenderPassMilliseconds);
	EXPECT_EQ(generatedVoxelCount, 141704u);
	// Exact boundary subtraction retains visible partial faces and may split one source
	// polygon into several remnants; exposed slope/stair inner faces are retained too.
	EXPECT_EQ(triangleCount, 785699u);
}

// Focused mesher budget: one representative chunk baked directly through
// VoxelMesher::buildRenderMesh, so throughput regressions surface without the full
// 8x8 scene, the worker pool, or command assembly. The grid replicates chunk (0, 0, 0)
// of the 8x8 scene.
TEST(VoxelRenderPerformance, SingleChunkBakeStaysWithinBudget)
{
	constexpr long long MaximumSingleChunkBakeMilliseconds = 25;
	constexpr int BakeCount = 10;

	spk::VoxelRegistry registry;
	const spk::VoxelRuntimeId cube = registry.registerShape(std::make_unique<spk::CubeVoxelShape>(spk::AtlasCell{0, 0}));
	const spk::VoxelRuntimeId slope = registry.registerShape(std::make_unique<spk::SlopeVoxelShape>(spk::AtlasCell{1, 0}));
	const spk::VoxelRuntimeId slab = registry.registerShape(std::make_unique<spk::SlabVoxelShape>(spk::AtlasCell{1, 0}));
	const spk::VoxelRuntimeId stair = registry.registerShape(std::make_unique<spk::StairVoxelShape>(spk::AtlasCell{2, 0}, 4));
	const spk::VoxelRuntimeId crossPlane = registry.registerShape(std::make_unique<spk::DiagonalCrossVoxelShape>(spk::AtlasCell{3, 0}));

	const spk::Perlin perlin(spk::Perlin::Parameters{.seed = 12345, .octaves = 4, .persistence = 0.5f, .lacunarity = 2.0f, .frequency = 1.0f / 32.0f});
	constexpr std::array orientations = {
		spk::VoxelOrientation::PositiveX,
		spk::VoxelOrientation::PositiveZ,
		spk::VoxelOrientation::NegativeX,
		spk::VoxelOrientation::NegativeZ};

	spk::VoxelGrid grid(spk::VoxelChunk::Size);
	for (int z = 0; z < spk::VoxelChunk::Size.z; ++z)
	{
		for (int x = 0; x < spk::VoxelChunk::Size.x; ++x)
		{
			const float rawHeight = perlin.sample2D(
				static_cast<float>(x),
				static_cast<float>(z),
				1.0f,
				static_cast<float>(spk::VoxelChunk::Size.y - 1));
			const int height = std::clamp(static_cast<int>(rawHeight), 1, spk::VoxelChunk::Size.y);

			for (int y = 0; y < height - 1; ++y)
			{
				const bool carveCave = y > 0 && positiveModulo(x * 31 + y * 17 + z * 13, 23) == 0;
				if (carveCave == false)
				{
					grid.cell(x, y, z) = {cube};
				}
			}

			const int topPattern = positiveModulo(x * 7 + z * 11, 8);
			const spk::VoxelOrientation orientation = orientations[static_cast<std::size_t>(positiveModulo(x + z, 4))];
			const spk::VoxelFlip flip = positiveModulo(x * 3 + z, 2) == 0
											? spk::VoxelFlip::PositiveY
											: spk::VoxelFlip::NegativeY;
			const spk::VoxelRuntimeId topShape = topPattern < 2 ? slab : topPattern < 4 ? slope
																	 : topPattern < 6	? stair
																						: cube;
			grid.cell(x, height - 1, z) = {topShape, orientation, flip};

			for (int y = height; y < spk::VoxelChunk::Size.y; ++y)
			{
				const int structurePattern = positiveModulo(x * 19 + y * 7 + z * 29, 13);
				if (structurePattern > 1)
				{
					continue;
				}
				const spk::VoxelRuntimeId shape = structurePattern == 0 ? crossPlane : stair;
				grid.cell(x, y, z) = {shape, orientations[static_cast<std::size_t>(positiveModulo(x + y + z, 4))], flip};
			}
		}
	}

	// Warm-up bake, also validating the grid produces real geometry.
	const spk::VoxelRenderMeshes warmup = spk::VoxelMesher::buildRenderMesh(grid, registry);
	const std::size_t triangleCount = (warmup.opaque.indexes().size() + warmup.transparent.indexes().size()) / 3;
	ASSERT_GT(triangleCount, 0u);

	spk::Chronometer chronometer;
	chronometer.start();
	for (int bake = 0; bake < BakeCount; ++bake)
	{
		const spk::VoxelRenderMeshes meshes = spk::VoxelMesher::buildRenderMesh(grid, registry);
		ASSERT_EQ((meshes.opaque.indexes().size() + meshes.transparent.indexes().size()) / 3, triangleCount);
	}
	const spk::Duration elapsed = chronometer.elapsedTime();
	const long long averageMicroseconds = elapsed.nanoseconds() / 1000 / BakeCount;

	EXPECT_LT(averageMicroseconds / 1000, MaximumSingleChunkBakeMilliseconds);
}
