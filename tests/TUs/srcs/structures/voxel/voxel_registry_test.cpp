#include <gtest/gtest.h>

#include <memory>
#include <stdexcept>
#include <utility>
#include <vector>

#include "structures/voxel/spk_cube_voxel_shape.hpp"
#include "structures/voxel/spk_slope_voxel_shape.hpp"
#include "structures/voxel/spk_voxel_grid.hpp"
#include "structures/voxel/spk_voxel_mesher.hpp"
#include "structures/voxel/spk_voxel_registry.hpp"

namespace
{
	// A shape whose initialization fails, to probe the registry's strong exception safety.
	class ThrowingShape final : public spk::CubeVoxelShape
	{
	public:
		ThrowingShape() :
			spk::CubeVoxelShape(spk::AtlasCell{0, 0})
		{
		}

	protected:
		void _constructRenderFaces() override
		{
			throw std::runtime_error("initialization failure");
		}
	};

	[[nodiscard]] std::vector<std::unique_ptr<spk::VoxelShape>> makeCubes(std::size_t p_count)
	{
		std::vector<std::unique_ptr<spk::VoxelShape>> shapes;
		for (std::size_t index = 0; index < p_count; ++index)
		{
			shapes.push_back(std::make_unique<spk::CubeVoxelShape>(spk::AtlasCell{static_cast<int>(index), 0}));
		}
		return shapes;
	}
}

TEST(VoxelRegistryTypes, RegistersOneStateType)
{
	spk::VoxelRegistry registry;
	const spk::VoxelTypeRegistration registration = registry.registerType(makeCubes(1));

	EXPECT_EQ(registration.type, spk::VoxelTypeId{0});
	ASSERT_EQ(registration.states.size(), 1u);
	EXPECT_EQ(registration.states.front(), spk::VoxelRuntimeId{0});
	EXPECT_EQ(registration.defaultRuntimeId(), spk::VoxelRuntimeId{0});
	EXPECT_EQ(registry.typeCount(), 1u);
	EXPECT_EQ(registry.runtimeStateCount(), 1u);
	EXPECT_EQ(registry.stateCount(registration.type), 1u);
}

TEST(VoxelRegistryTypes, RegistersMultiStateTypeWithDenseContiguousIds)
{
	spk::VoxelRegistry registry;
	const spk::VoxelTypeRegistration first = registry.registerType(makeCubes(2));
	const spk::VoxelTypeRegistration second = registry.registerType(makeCubes(3));

	// Type ids stay dense.
	EXPECT_EQ(first.type, spk::VoxelTypeId{0});
	EXPECT_EQ(second.type, spk::VoxelTypeId{1});
	EXPECT_EQ(registry.typeCount(), 2u);

	// Runtime ids stay dense across types.
	ASSERT_EQ(first.states.size(), 2u);
	ASSERT_EQ(second.states.size(), 3u);
	EXPECT_EQ(first.states[0], spk::VoxelRuntimeId{0});
	EXPECT_EQ(first.states[1], spk::VoxelRuntimeId{1});
	EXPECT_EQ(second.states[0], spk::VoxelRuntimeId{2});
	EXPECT_EQ(second.states[1], spk::VoxelRuntimeId{3});
	EXPECT_EQ(second.states[2], spk::VoxelRuntimeId{4});
	EXPECT_EQ(registry.runtimeStateCount(), 5u);
	EXPECT_EQ(registry.size(), 5u);

	// State ids start at zero and are contiguous.
	for (std::size_t index = 0; index < second.states.size(); ++index)
	{
		const spk::VoxelStateReference &reference = registry.stateReference(second.states[index]);
		EXPECT_EQ(reference.type, second.type);
		EXPECT_EQ(reference.state, spk::VoxelStateId{static_cast<std::uint16_t>(index)});
	}
}

TEST(VoxelRegistryTypes, MapsRuntimeAndStateInBothDirections)
{
	spk::VoxelRegistry registry;
	const spk::VoxelTypeRegistration registration = registry.registerType(makeCubes(3));

	for (std::size_t index = 0; index < registration.states.size(); ++index)
	{
		const spk::VoxelStateId state{static_cast<std::uint16_t>(index)};
		EXPECT_EQ(registry.runtimeId(registration.type, state), registration.states[index]);
		const spk::VoxelStateReference &reference = registry.stateReference(registration.states[index]);
		EXPECT_EQ(registry.runtimeId(reference.type, reference.state), registration.states[index]);
	}
}

TEST(VoxelRegistryTypes, RejectsInvalidLookups)
{
	spk::VoxelRegistry registry;
	(void)registry.registerType(makeCubes(2));

	// Invalid runtime lookups.
	EXPECT_EQ(registry.tryShape(spk::VoxelRuntimeId{}), nullptr);
	EXPECT_EQ(registry.tryShape(spk::VoxelRuntimeId{2}), nullptr);
	EXPECT_EQ(registry.tryStateReference(spk::VoxelRuntimeId{}), nullptr);
	EXPECT_EQ(registry.tryStateReference(spk::VoxelRuntimeId{2}), nullptr);
	EXPECT_THROW((void)registry.shape(spk::VoxelRuntimeId{2}), std::out_of_range);
	EXPECT_THROW((void)registry.stateReference(spk::VoxelRuntimeId{2}), std::out_of_range);

	// Invalid type and state lookups.
	EXPECT_EQ(registry.tryRuntimeId(spk::VoxelTypeId{1}, spk::VoxelStateId{0}), std::nullopt);
	EXPECT_EQ(registry.tryRuntimeId(spk::VoxelTypeId{0}, spk::VoxelStateId{2}), std::nullopt);
	EXPECT_THROW((void)registry.runtimeId(spk::VoxelTypeId{1}, spk::VoxelStateId{0}), std::out_of_range);
	EXPECT_THROW((void)registry.runtimeId(spk::VoxelTypeId{0}, spk::VoxelStateId{2}), std::out_of_range);
	EXPECT_THROW((void)registry.stateCount(spk::VoxelTypeId{1}), std::out_of_range);
}

TEST(VoxelRegistryTypes, RejectsEmptyTypesAndNullShapes)
{
	spk::VoxelRegistry registry;

	EXPECT_THROW((void)registry.registerType({}), std::invalid_argument);

	std::vector<std::unique_ptr<spk::VoxelShape>> withNull = makeCubes(1);
	withNull.push_back(nullptr);
	EXPECT_THROW((void)registry.registerType(std::move(withNull)), std::invalid_argument);

	EXPECT_EQ(registry.typeCount(), 0u);
	EXPECT_EQ(registry.runtimeStateCount(), 0u);
}

TEST(VoxelRegistryTypes, FailedRegistrationLeavesTheRegistryUntouched)
{
	spk::VoxelRegistry registry;
	const spk::VoxelTypeRegistration existing = registry.registerType(makeCubes(2));

	std::vector<std::unique_ptr<spk::VoxelShape>> failing = makeCubes(1);
	failing.push_back(std::make_unique<ThrowingShape>());
	EXPECT_THROW((void)registry.registerType(std::move(failing)), std::runtime_error);

	// No partial type, no orphaned shapes, no mismatched mapping vectors.
	EXPECT_EQ(registry.typeCount(), 1u);
	EXPECT_EQ(registry.runtimeStateCount(), 2u);
	EXPECT_EQ(registry.tryShape(spk::VoxelRuntimeId{2}), nullptr);
	EXPECT_EQ(registry.tryStateReference(spk::VoxelRuntimeId{2}), nullptr);

	// The registry keeps attributing dense ids afterwards.
	const spk::VoxelTypeRegistration next = registry.registerType(makeCubes(1));
	EXPECT_EQ(next.type, spk::VoxelTypeId{1});
	EXPECT_EQ(next.states.front(), spk::VoxelRuntimeId{2});
	(void)existing;
}

TEST(VoxelRegistryTypes, LegacyRegisterShapeCreatesAOneStateType)
{
	spk::VoxelRegistry registry;
	const spk::VoxelRuntimeId runtime = registry.registerShape(
		std::make_unique<spk::CubeVoxelShape>(spk::AtlasCell{0, 0}));

	EXPECT_EQ(runtime, spk::VoxelRuntimeId{0});
	EXPECT_EQ(registry.typeCount(), 1u);
	EXPECT_EQ(registry.stateCount(spk::VoxelTypeId{0}), 1u);
	const spk::VoxelStateReference &reference = registry.stateReference(runtime);
	EXPECT_EQ(reference.type, spk::VoxelTypeId{0});
	EXPECT_EQ(reference.state, spk::VoxelStateId{0});
	EXPECT_TRUE(registry.shape(runtime).initialized());
}

TEST(VoxelRegistryTypes, ShapeLookupIsDirectByRuntimeId)
{
	spk::VoxelRegistry registry;
	const spk::VoxelTypeRegistration registration = registry.registerType(makeCubes(2));

	const spk::VoxelShape *first = registry.tryShape(registration.states[0]);
	const spk::VoxelShape *second = registry.tryShape(registration.states[1]);
	ASSERT_NE(first, nullptr);
	ASSERT_NE(second, nullptr);
	EXPECT_NE(first, second);
	EXPECT_EQ(&registry.shape(registration.states[0]), first);
	EXPECT_EQ(&registry.shape(registration.states[1]), second);
}

TEST(VoxelRegistryTypes, MesherRendersTheStateSelectedByRuntimeId)
{
	// Two states of one type using different atlas cells: the mesher must emit the
	// UVs of exactly the state the cell's runtime id selects.
	spk::VoxelRegistry registry;
	const spk::VoxelTypeRegistration registration = registry.registerType(makeCubes(2));

	const auto meshFor = [&](spk::VoxelRuntimeId p_runtime) {
		spk::VoxelGrid grid({1, 1, 1});
		grid.cell(0, 0, 0) = {p_runtime};
		return spk::VoxelMesher::buildRenderMesh(grid, registry);
	};

	const spk::VoxelRenderMeshes defaultState = meshFor(registration.states[0]);
	const spk::VoxelRenderMeshes alternateState = meshFor(registration.states[1]);

	ASSERT_FALSE(defaultState.opaque.vertices().empty());
	ASSERT_EQ(defaultState.opaque.vertices().size(), alternateState.opaque.vertices().size());

	bool foundUvDifference = false;
	for (std::size_t index = 0; index < defaultState.opaque.vertices().size(); ++index)
	{
		EXPECT_EQ(
			defaultState.opaque.vertices()[index].position,
			alternateState.opaque.vertices()[index].position);
		if (defaultState.opaque.vertices()[index].uv != alternateState.opaque.vertices()[index].uv)
		{
			foundUvDifference = true;
		}
	}
	EXPECT_TRUE(foundUvDifference);
}

TEST(VoxelRegistryTypes, OrientationAndFlipApplyIndependentlyOfState)
{
	// Orientation and flip are transformations of the current state, not states: the
	// same rotation applied to two states of one type must produce identical geometry
	// (only the textures may differ), and rotating one state must move its geometry.
	spk::VoxelRegistry registry;
	std::vector<std::unique_ptr<spk::VoxelShape>> slopes;
	slopes.push_back(std::make_unique<spk::SlopeVoxelShape>(spk::AtlasCell{0, 0}));
	slopes.push_back(std::make_unique<spk::SlopeVoxelShape>(spk::AtlasCell{1, 0}));
	const spk::VoxelTypeRegistration registration = registry.registerType(std::move(slopes));

	const auto meshFor = [&](spk::VoxelRuntimeId p_runtime,
							 spk::VoxelOrientation p_orientation,
							 spk::VoxelFlip p_flip) {
		spk::VoxelGrid grid({1, 1, 1});
		grid.cell(0, 0, 0) = {p_runtime, p_orientation, p_flip};
		return spk::VoxelMesher::buildRenderMesh(grid, registry);
	};

	const spk::VoxelRenderMeshes defaultRotated =
		meshFor(registration.states[0], spk::VoxelOrientation::PositiveX, spk::VoxelFlip::NegativeY);
	const spk::VoxelRenderMeshes alternateRotated =
		meshFor(registration.states[1], spk::VoxelOrientation::PositiveX, spk::VoxelFlip::NegativeY);
	ASSERT_EQ(defaultRotated.opaque.vertices().size(), alternateRotated.opaque.vertices().size());
	for (std::size_t index = 0; index < defaultRotated.opaque.vertices().size(); ++index)
	{
		EXPECT_EQ(
			defaultRotated.opaque.vertices()[index].position,
			alternateRotated.opaque.vertices()[index].position);
	}

	const spk::VoxelRenderMeshes alternateUpright =
		meshFor(registration.states[1], spk::VoxelOrientation::PositiveZ, spk::VoxelFlip::PositiveY);
	bool foundPositionDifference =
		alternateUpright.opaque.vertices().size() != alternateRotated.opaque.vertices().size();
	for (std::size_t index = 0;
		 !foundPositionDifference && index < alternateRotated.opaque.vertices().size();
		 ++index)
	{
		foundPositionDifference = alternateRotated.opaque.vertices()[index].position !=
								  alternateUpright.opaque.vertices()[index].position;
	}
	EXPECT_TRUE(foundPositionDifference);
}
