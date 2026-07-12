#include <gtest/gtest.h>

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <set>
#include <stdexcept>
#include <string>
#include <vector>

#include "structures/voxel/spk_cube_voxel_shape.hpp"
#include "structures/voxel/spk_voxel_cell_lookup.hpp"
#include "structures/voxel/spk_voxel_fluid.hpp"
#include "structures/voxel/spk_voxel_mesher.hpp"
#include "structures/voxel/spk_voxel_registry.hpp"

namespace
{
	[[nodiscard]] spk::VoxelFluidDescription makeDescription(std::size_t p_levelCount, float p_transparency = 0.5f)
	{
		return spk::VoxelFluidDescription{
			.levelCount = p_levelCount,
			.falls = true,
			.appearance = spk::VoxelFluidAppearance{
				.topTexture = {2, 1},
				.bottomTexture = {2, 1},
				.sideTexture = {2, 1},
				.transparency = p_transparency}};
	}

	// One registry with an ordinary opaque type, a transparent non-fluid material and one
	// eight-level fluid family, mirroring a typical game catalog.
	struct FluidRegistry
	{
		spk::VoxelRegistry registry;
		spk::VoxelRuntimeId cube{};
		spk::VoxelRuntimeId glass{};
		std::size_t family = 0;

		explicit FluidRegistry(std::size_t p_levelCount = 8)
		{
			cube = registry.registerShape(std::make_unique<spk::CubeVoxelShape>(spk::AtlasCell{0, 0}));
			auto glassShape = std::make_unique<spk::CubeVoxelShape>(spk::AtlasCell{1, 0});
			glassShape->setTransparency(0.25f);
			glassShape->setTransparentOcclusionGroup("glass");
			glass = registry.registerShape(std::move(glassShape));
			family = registry.registerFluid(makeDescription(p_levelCount));
		}

		[[nodiscard]] const spk::VoxelFluidFamily &fluid() const
		{
			return registry.fluidFamily(family);
		}
	};

	class SingleCellLookup final : public spk::IVoxelCellLookup
	{
	private:
		spk::Vector3Int _target;
		spk::VoxelCell _cell;

	public:
		SingleCellLookup(const spk::Vector3Int &p_target, spk::VoxelRuntimeId p_id) :
			_target(p_target),
			_cell{p_id}
		{
		}

		[[nodiscard]] const spk::VoxelCell *tryCell(const spk::Vector3Int &p_worldCell) const override
		{
			return p_worldCell == _target ? &_cell : nullptr;
		}
	};

	[[nodiscard]] bool meshHasVertexAt(const spk::VoxelMesh3D &p_mesh, float p_y, const spk::Vector3 &p_normal)
	{
		for (const spk::VoxelVertex3D &vertex : p_mesh.vertices())
		{
			if (vertex.normal == p_normal && std::abs(vertex.position.y - p_y) <= 0.0001f)
			{
				return true;
			}
		}
		return false;
	}
}

// ---------------------------------------------------------------------------
// Registration: one semantic type, generated states
// ---------------------------------------------------------------------------

TEST(VoxelFluidRegistry, OneRegistrationCreatesOneTypeWithGeneratedStates)
{
	const FluidRegistry test(8);

	// One semantic type for the whole family, holding N + 1 states (source + levels).
	EXPECT_EQ(test.registry.typeCount(), 3u);
	const spk::VoxelFluidFamily &family = test.fluid();
	EXPECT_EQ(test.registry.stateCount(family.type), 9u);
	EXPECT_EQ(test.registry.runtimeStateCount(), 2u + 9u);
	EXPECT_TRUE(test.registry.hasFluids());
	EXPECT_EQ(test.registry.fluidFamilies().size(), 1u);
}

TEST(VoxelFluidRegistry, SourceIsStateZeroAndLevelsUseStatesOneThroughN)
{
	const FluidRegistry test(8);
	const spk::VoxelFluidFamily &family = test.fluid();

	EXPECT_EQ(family.sourceState, spk::VoxelStateId{0});
	EXPECT_EQ(family.sourceRuntime, test.registry.runtimeId(family.type, spk::VoxelStateId{0}));
	ASSERT_EQ(family.levelCount(), 8u);

	std::set<spk::VoxelRuntimeId> runtimes{family.sourceRuntime};
	for (std::size_t level = 1; level <= family.levelCount(); ++level)
	{
		const spk::VoxelFluidState &state = family.level(level);
		EXPECT_EQ(state.level, level);
		EXPECT_EQ(state.state, (spk::VoxelStateId{static_cast<std::uint16_t>(level)}));
		EXPECT_EQ(state.runtime, test.registry.runtimeId(family.type, state.state));
		EXPECT_FLOAT_EQ(state.height, static_cast<float>(level) / 8.0f);
		runtimes.insert(state.runtime);
	}
	// Every state owns a distinct runtime id.
	EXPECT_EQ(runtimes.size(), 9u);

	EXPECT_THROW((void)family.level(0), std::out_of_range);
	EXPECT_THROW((void)family.level(9), std::out_of_range);
}

TEST(VoxelFluidRegistry, RuntimeClassificationIsCorrect)
{
	const FluidRegistry test(4);
	const spk::VoxelFluidFamily &family = test.fluid();

	const spk::VoxelFluidRef *source = test.registry.tryFluidRef(family.sourceRuntime);
	ASSERT_NE(source, nullptr);
	EXPECT_TRUE(source->source);
	EXPECT_EQ(source->family, test.family);
	EXPECT_EQ(source->type, family.type);
	EXPECT_EQ(source->state, spk::VoxelStateId{0});
	EXPECT_EQ(source->runtime, family.sourceRuntime);
	EXPECT_EQ(source->level, 4u); // a source pours at full strength

	for (std::size_t level = 1; level <= family.levelCount(); ++level)
	{
		const spk::VoxelFluidRef *reference = test.registry.tryFluidRef(family.level(level).runtime);
		ASSERT_NE(reference, nullptr);
		EXPECT_FALSE(reference->source);
		EXPECT_EQ(reference->level, level);
		EXPECT_EQ(reference->family, test.family);
	}

	// Ordinary states report no fluid classification, invalid ids neither.
	EXPECT_EQ(test.registry.tryFluidRef(test.cube), nullptr);
	EXPECT_EQ(test.registry.tryFluidRef(test.glass), nullptr);
	EXPECT_EQ(test.registry.tryFluidRef(spk::VoxelRuntimeId{}), nullptr);
	EXPECT_EQ(test.registry.tryFluidRef(spk::VoxelRuntimeId{9999}), nullptr);
}

TEST(VoxelFluidRegistry, InvalidDescriptionsFailWithoutMutatingTheRegistry)
{
	spk::VoxelRegistry registry;
	const spk::VoxelRuntimeId cube = registry.registerShape(std::make_unique<spk::CubeVoxelShape>(spk::AtlasCell{0, 0}));

	EXPECT_THROW((void)registry.registerFluid(makeDescription(0)), std::invalid_argument);
	EXPECT_THROW((void)registry.registerFluid(makeDescription(8, -0.5f)), std::invalid_argument);
	EXPECT_THROW((void)registry.registerFluid(makeDescription(8, 1.0f)), std::invalid_argument);

	// A failed registration leaves the registry untouched...
	EXPECT_EQ(registry.typeCount(), 1u);
	EXPECT_EQ(registry.runtimeStateCount(), 1u);
	EXPECT_FALSE(registry.hasFluids());
	EXPECT_EQ(registry.tryFluidRef(cube), nullptr);

	// ...and a later valid one lands as family 0 with the expected type id.
	const std::size_t family = registry.registerFluid(makeDescription(2));
	EXPECT_EQ(family, 0u);
	EXPECT_EQ(registry.fluidFamily(family).type.index(), 1u);
	EXPECT_THROW((void)registry.fluidFamily(1), std::out_of_range);
}

TEST(VoxelFluidRegistry, SourceAndStrongestLevelShareFullHeightButStayDistinct)
{
	const FluidRegistry test(8);
	const spk::VoxelFluidFamily &family = test.fluid();
	const spk::VoxelFluidState &strongest = family.level(family.levelCount());

	EXPECT_NE(family.sourceRuntime, strongest.runtime);
	EXPECT_NE(family.sourceState, strongest.state);
	EXPECT_FLOAT_EQ(strongest.height, 1.0f);

	// Both render as full cubes: their top face lies on and covers the +Y boundary.
	for (const spk::VoxelRuntimeId runtime : {family.sourceRuntime, strongest.runtime})
	{
		const spk::VoxelShape &shape = test.registry.shape(runtime);
		EXPECT_TRUE(shape.outerFaceCoversCellBoundary(spk::VoxelAxisPlane::PositiveY));
		EXPECT_TRUE(shape.outerFaceCoversCellBoundary(spk::VoxelAxisPlane::NegativeY));
	}

	// A partial level's top sits below the boundary: no outer +Y face at all.
	const spk::VoxelShape &half = test.registry.shape(family.level(4).runtime);
	EXPECT_FALSE(half.renderFaces().outer(spk::VoxelAxisPlane::PositiveY).has_value());
	EXPECT_TRUE(half.outerFaceCoversCellBoundary(spk::VoxelAxisPlane::NegativeY));
}

TEST(VoxelFluidRegistry, AllStatesOfOneFamilyShareOneOcclusionGroup)
{
	const FluidRegistry test(8);
	const spk::VoxelFluidFamily &family = test.fluid();

	const std::string &group = test.registry.shape(family.sourceRuntime).transparentOcclusionGroup();
	EXPECT_FALSE(group.empty());
	for (std::size_t level = 1; level <= family.levelCount(); ++level)
	{
		EXPECT_EQ(test.registry.shape(family.level(level).runtime).transparentOcclusionGroup(), group);
	}
	EXPECT_NE(group, test.registry.shape(test.glass).transparentOcclusionGroup());
}

TEST(VoxelFluidRegistry, DifferentFamiliesUseDifferentGroups)
{
	spk::VoxelRegistry registry;
	const std::size_t water = registry.registerFluid(makeDescription(8));
	const std::size_t lava = registry.registerFluid(makeDescription(4, 0.25f));

	const std::string &waterGroup = registry.shape(registry.fluidFamily(water).sourceRuntime).transparentOcclusionGroup();
	const std::string &lavaGroup = registry.shape(registry.fluidFamily(lava).sourceRuntime).transparentOcclusionGroup();
	EXPECT_FALSE(waterGroup.empty());
	EXPECT_FALSE(lavaGroup.empty());
	EXPECT_NE(waterGroup, lavaGroup);
}

// ---------------------------------------------------------------------------
// Rendering: transparent partial occlusion between fluid states
// ---------------------------------------------------------------------------

TEST(VoxelFluidRendering, EqualAdjacentLevelsCullTheirSharedSide)
{
	const FluidRegistry test(8);
	const spk::VoxelRuntimeId half = test.fluid().level(4).runtime;

	// One lone partial level: inner top + bottom + four sides.
	const spk::VoxelGrid lone({1, 1, 1}, {half});
	EXPECT_EQ(spk::VoxelMesher::buildRenderMesh(lone, test.registry).transparent.nbShape(), 6u);

	spk::VoxelGrid grid({2, 1, 1});
	grid.cell(0, 0, 0) = {half};
	grid.cell(1, 0, 0) = {half};
	// Both shared side quads disappear: 12 - 2.
	EXPECT_EQ(spk::VoxelMesher::buildRenderMesh(grid, test.registry).transparent.nbShape(), 10u);
}

TEST(VoxelFluidRendering, TallerBesideShorterKeepsOnlyTheExposedStrip)
{
	const FluidRegistry test(8);
	spk::VoxelGrid grid({2, 1, 1});
	grid.cell(0, 0, 0) = {test.fluid().level(6).runtime}; // h = 0.75
	grid.cell(1, 0, 0) = {test.fluid().level(2).runtime}; // h = 0.25

	const spk::VoxelMesh3D &mesh = spk::VoxelMesher::buildRenderMesh(grid, test.registry).transparent;
	// The shorter side quad is fully covered (gone); the taller one keeps only the strip
	// between the two fill heights: 12 quads - 2 shared + the strip remnant.
	EXPECT_EQ(mesh.nbShape(), 11u);

	std::vector<float> stripY;
	for (const spk::VoxelVertex3D &vertex : mesh.vertices())
	{
		if (vertex.normal == spk::Vector3(1.0f, 0.0f, 0.0f) && vertex.position.x == 1.0f)
		{
			stripY.push_back(vertex.position.y);
		}
	}
	std::ranges::sort(stripY);
	ASSERT_EQ(stripY.size(), 4u);
	EXPECT_FLOAT_EQ(stripY[0], 0.25f);
	EXPECT_FLOAT_EQ(stripY[1], 0.25f);
	EXPECT_FLOAT_EQ(stripY[2], 0.75f);
	EXPECT_FLOAT_EQ(stripY[3], 0.75f);

	// The shorter level keeps its own top surface.
	EXPECT_TRUE(meshHasVertexAt(mesh, 0.25f, {0.0f, 1.0f, 0.0f}));
}

TEST(VoxelFluidRendering, VerticallyAdjacentFullStatesCullTheInternalBoundary)
{
	const FluidRegistry test(8);
	spk::VoxelGrid grid({1, 2, 1});
	grid.cell(0, 0, 0) = {test.fluid().sourceRuntime};
	grid.cell(0, 1, 0) = {test.fluid().level(8).runtime};

	// Two full cubes of the same medium: 12 faces minus the two internal ones.
	EXPECT_EQ(spk::VoxelMesher::buildRenderMesh(grid, test.registry).transparent.nbShape(), 10u);
}

TEST(VoxelFluidRendering, FluidBesideAnotherTransparentMaterialKeepsTheInterface)
{
	const FluidRegistry test(8);
	spk::VoxelGrid grid({2, 1, 1});
	grid.cell(0, 0, 0) = {test.fluid().sourceRuntime};
	grid.cell(1, 0, 0) = {test.glass};

	// Different occlusion groups: both interface faces stay (6 + 6).
	EXPECT_EQ(spk::VoxelMesher::buildRenderMesh(grid, test.registry).transparent.nbShape(), 12u);
}

TEST(VoxelFluidRendering, TwoFamiliesKeepTheirInterface)
{
	spk::VoxelRegistry registry;
	const std::size_t water = registry.registerFluid(makeDescription(8));
	const std::size_t lava = registry.registerFluid(makeDescription(8, 0.5f));
	spk::VoxelGrid grid({2, 1, 1});
	grid.cell(0, 0, 0) = {registry.fluidFamily(water).sourceRuntime};
	grid.cell(1, 0, 0) = {registry.fluidFamily(lava).sourceRuntime};

	EXPECT_EQ(spk::VoxelMesher::buildRenderMesh(grid, registry).transparent.nbShape(), 12u);
}

TEST(VoxelFluidRendering, CrossChunkFluidBoundariesBehaveLikeInternalOnes)
{
	const FluidRegistry test(8);
	const spk::VoxelGrid grid({1, 1, 1}, {test.fluid().sourceRuntime});

	// A same-family source just across the grid boundary culls the shared face exactly
	// like an in-grid neighbor would.
	const SingleCellLookup neighborSource({1, 0, 0}, test.fluid().sourceRuntime);
	EXPECT_EQ(spk::VoxelMesher::buildRenderMesh(grid, test.registry, neighborSource, {0, 0, 0}).transparent.nbShape(), 5u);

	// The same neighbor from another family keeps the interface.
	spk::VoxelRegistry twoFamilies;
	const std::size_t water = twoFamilies.registerFluid(makeDescription(8));
	const std::size_t lava = twoFamilies.registerFluid(makeDescription(8, 0.5f));
	const spk::VoxelGrid waterGrid({1, 1, 1}, {twoFamilies.fluidFamily(water).sourceRuntime});
	const SingleCellLookup lavaNeighbor({1, 0, 0}, twoFamilies.fluidFamily(lava).sourceRuntime);
	EXPECT_EQ(spk::VoxelMesher::buildRenderMesh(waterGrid, twoFamilies, lavaNeighbor, {0, 0, 0}).transparent.nbShape(), 6u);
}

TEST(VoxelFluidRendering, SideUVsAreCroppedToTheFillHeight)
{
	const FluidRegistry test(8);
	const spk::VoxelShape &quarter = test.registry.shape(test.fluid().level(2).runtime); // h = 0.25

	const auto &sideFace = quarter.renderFaces().outer(spk::VoxelAxisPlane::PositiveX);
	ASSERT_TRUE(sideFace.has_value());
	ASSERT_EQ(sideFace->polygons().size(), 1u);
	// Local V runs from 1 at the bottom edge to 1 - h at the fill line; the shape stores
	// atlas UVs, so compare against the atlas-mapped values of the "side" cell.
	float minV = 1.0f;
	float maxV = 0.0f;
	const spk::VoxelShapePolygon &polygon = sideFace->polygons().front();
	for (std::size_t index = 0; index < polygon.size(); ++index)
	{
		minV = std::min(minV, polygon[index].data.y);
		maxV = std::max(maxV, polygon[index].data.y);
	}
	const spk::Vector2 bottomUV = quarter.atlasUV({2, 1}, {0.0f, 1.0f});
	const spk::Vector2 topUV = quarter.atlasUV({2, 1}, {0.0f, 0.75f});
	EXPECT_NEAR(maxV, bottomUV.y, 0.0001f);
	EXPECT_NEAR(minV, topUV.y, 0.0001f);
}
