#include <gtest/gtest.h>

#include "voxel/shape_catalog.hpp"
#include "voxel/voxel_definition.hpp"
#include "voxel/voxel_registry.hpp"

#include "structures/voxel/spk_voxel_mesher.hpp"

#include <filesystem>
#include <fstream>
#include <string>
#include <string_view>
#include <vector>

namespace
{
	[[nodiscard]] const pg::ShapeCatalog &loadedShapes()
	{
		static const pg::ShapeCatalog shapes = [] {
			pg::ShapeCatalog result;
			spk::loadJsonDirectory(
				result,
				std::filesystem::path(PG_RESOURCE_DIR) / "data" / "shapes",
				[](std::string_view p_id, pg::JsonReader &p_reader) {
					pg::ShapeDefinition definition = pg::parseShapeDefinition(p_reader);
					definition.id = p_id;
					return definition;
				});
			return result;
		}();
		return shapes;
	}

	// The full resource catalog, loaded once: also proves every shipped voxel file still
	// parses under the type/state model.
	[[nodiscard]] const pg::VoxelRegistry &loadedVoxels()
	{
		static const pg::VoxelRegistry registry = [] {
			pg::VoxelRegistry result;
			result.load(loadedShapes(), std::filesystem::path(PG_RESOURCE_DIR) / "data" / "voxels");
			return result;
		}();
		return registry;
	}

	[[nodiscard]] pg::ParsedVoxel parseFromText(const std::string &p_json)
	{
		const spk::JSON::Value value = spk::JSON::Value::fromString(p_json);
		pg::JsonReader reader(value, "<test>");
		return pg::parseVoxelDefinition(reader, loadedShapes());
	}

	// A scratch voxel directory for end-to-end load() tests.
	class VoxelDirectory
	{
	private:
		std::filesystem::path _directory;

	public:
		VoxelDirectory()
		{
			_directory = std::filesystem::path(testing::TempDir()) /
						 ("pg-voxels-" + std::to_string(::testing::UnitTest::GetInstance()->random_seed()) + "-" +
						  ::testing::UnitTest::GetInstance()->current_test_info()->name());
			std::filesystem::create_directories(_directory);
		}

		~VoxelDirectory()
		{
			std::error_code error;
			std::filesystem::remove_all(_directory, error);
		}

		void add(const std::string &p_name, const std::string &p_json) const
		{
			std::ofstream stream(_directory / (p_name + ".json"), std::ios::binary);
			stream << p_json;
		}

		[[nodiscard]] const std::filesystem::path &path() const noexcept
		{
			return _directory;
		}
	};

	constexpr const char *kTwoStateBush = R"({
		"version": 3,
		"traversal": "passable",
		"tags": ["Bush", "flora"],
		"shape": "cross-plane",
		"states": [
			{ "id": 0, "name": "default", "textures": { "plane": [0, 2] } },
			{ "id": 1, "name": "alternate", "textures": { "plane": [1, 2] } }
		]
	})";
}

// ---------------------------------------------------------------------------
// Parsing: version 2 compatibility
// ---------------------------------------------------------------------------

TEST(VoxelParsing, VersionTwoBecomesOneDefaultState)
{
	const pg::ParsedVoxel voxel = parseFromText(R"({
		"version": 2,
		"traversal": "passable",
		"tags": ["Bush"],
		"shape": "cross-plane",
		"textures": { "plane": [0, 2] }
	})");

	ASSERT_EQ(voxel.states.size(), 1u);
	EXPECT_EQ(voxel.states.front().id, spk::VoxelStateId{0});
	EXPECT_EQ(voxel.states.front().name, "default");
	EXPECT_NE(voxel.states.front().shape, nullptr);
	EXPECT_EQ(voxel.data.traversal, pg::VoxelTraversal::Passable);
	EXPECT_FALSE(voxel.fluid.has_value());
}

TEST(VoxelParsing, VersionTwoRejectsStates)
{
	EXPECT_THROW(
		(void)parseFromText(R"({
			"version": 2,
			"traversal": "passable",
			"tags": [],
			"shape": "cross-plane",
			"textures": { "plane": [0, 2] },
			"states": []
		})"),
		pg::JsonError);
}

// ---------------------------------------------------------------------------
// Parsing: version 3 validation
// ---------------------------------------------------------------------------

TEST(VoxelParsing, VersionThreeRequiresStates)
{
	EXPECT_THROW(
		(void)parseFromText(R"({
			"version": 3,
			"traversal": "passable",
			"tags": [],
			"shape": "cross-plane"
		})"),
		pg::JsonError);
}

TEST(VoxelParsing, VersionThreeRequiresAtLeastOneState)
{
	EXPECT_THROW(
		(void)parseFromText(R"({
			"version": 3,
			"traversal": "passable",
			"tags": [],
			"shape": "cross-plane",
			"states": []
		})"),
		pg::JsonError);
}

TEST(VoxelParsing, VersionThreeRequiresStateZero)
{
	EXPECT_THROW(
		(void)parseFromText(R"({
			"version": 3,
			"traversal": "passable",
			"tags": [],
			"shape": "cross-plane",
			"states": [ { "id": 1, "textures": { "plane": [0, 2] } } ]
		})"),
		pg::JsonError);
}

TEST(VoxelParsing, VersionThreeRejectsDuplicateStateIds)
{
	EXPECT_THROW(
		(void)parseFromText(R"({
			"version": 3,
			"traversal": "passable",
			"tags": [],
			"shape": "cross-plane",
			"states": [
				{ "id": 0, "textures": { "plane": [0, 2] } },
				{ "id": 0, "textures": { "plane": [1, 2] } }
			]
		})"),
		pg::JsonError);
}

TEST(VoxelParsing, VersionThreeRejectsGapsInStateIds)
{
	EXPECT_THROW(
		(void)parseFromText(R"({
			"version": 3,
			"traversal": "passable",
			"tags": [],
			"shape": "cross-plane",
			"states": [
				{ "id": 0, "textures": { "plane": [0, 2] } },
				{ "id": 2, "textures": { "plane": [1, 2] } }
			]
		})"),
		pg::JsonError);
}

TEST(VoxelParsing, VersionThreeRejectsDuplicateNames)
{
	EXPECT_THROW(
		(void)parseFromText(R"({
			"version": 3,
			"traversal": "passable",
			"tags": [],
			"shape": "cross-plane",
			"states": [
				{ "id": 0, "name": "same", "textures": { "plane": [0, 2] } },
				{ "id": 1, "name": "same", "textures": { "plane": [1, 2] } }
			]
		})"),
		pg::JsonError);
}

TEST(VoxelParsing, VersionThreeRejectsEmptyNames)
{
	EXPECT_THROW(
		(void)parseFromText(R"({
			"version": 3,
			"traversal": "passable",
			"tags": [],
			"shape": "cross-plane",
			"states": [ { "id": 0, "name": "", "textures": { "plane": [0, 2] } } ]
		})"),
		pg::JsonError);
}

TEST(VoxelParsing, VersionThreeRejectsMissingTextureSlot)
{
	EXPECT_THROW(
		(void)parseFromText(R"({
			"version": 3,
			"traversal": "solid",
			"tags": [],
			"shape": "cube",
			"states": [ { "id": 0, "textures": { "top": [0, 0], "bottom": [2, 0] } } ]
		})"),
		pg::JsonError);
}

TEST(VoxelParsing, VersionThreeRejectsExtraTextureSlot)
{
	EXPECT_THROW(
		(void)parseFromText(R"({
			"version": 3,
			"traversal": "passable",
			"tags": [],
			"shape": "cross-plane",
			"states": [ { "id": 0, "textures": { "plane": [0, 2], "extra": [1, 2] } } ]
		})"),
		pg::JsonError);
}

TEST(VoxelParsing, VersionThreeForbidsTopLevelTextures)
{
	EXPECT_THROW(
		(void)parseFromText(R"({
			"version": 3,
			"traversal": "passable",
			"tags": [],
			"shape": "cross-plane",
			"textures": { "plane": [0, 2] },
			"states": [ { "id": 0, "textures": { "plane": [0, 2] } } ]
		})"),
		pg::JsonError);
}

TEST(VoxelParsing, VersionThreeForbidsFluidWithAuthoredStates)
{
	EXPECT_THROW(
		(void)parseFromText(R"({
			"version": 3,
			"traversal": "solid",
			"tags": [],
			"shape": "cube",
			"fluid": { "maxSpread": 4 },
			"states": [ { "id": 0, "textures": { "top": [0, 0], "bottom": [2, 0], "side": [1, 0] } } ]
		})"),
		pg::JsonError);
}

TEST(VoxelParsing, SharedShapeDataIsAppliedToEveryState)
{
	const pg::ParsedVoxel voxel = parseFromText(R"({
		"version": 3,
		"traversal": "solid",
		"tags": ["ground"],
		"shape": "cube",
		"states": [
			{ "id": 0, "name": "default", "textures": { "top": [0, 0], "bottom": [2, 0], "side": [1, 0] } },
			{ "id": 1, "name": "mossy", "textures": { "top": [3, 0], "bottom": [2, 0], "side": [4, 0] } }
		]
	})");

	ASSERT_EQ(voxel.states.size(), 2u);
	const pg::ShapeDefinition &cube = loadedShapes().get("cube");
	for (const pg::ParsedVoxelState &state : voxel.states)
	{
		EXPECT_NE(state.shape, nullptr);
		// Every ordinary state receives the shared shape's cardinal heights.
		EXPECT_EQ(state.heights.positiveY.stationary, cube.heights.positiveY.stationary);
		EXPECT_EQ(state.heights.positiveY.positiveX, cube.heights.positiveY.positiveX);
	}
	EXPECT_EQ(voxel.states[0].name, "default");
	EXPECT_EQ(voxel.states[1].name, "mossy");
}

// ---------------------------------------------------------------------------
// Registry: type/state/runtime resolution
// ---------------------------------------------------------------------------

TEST(VoxelRegistryStates, StringIdResolvesToOneTypeAndDefaultState)
{
	const pg::VoxelRegistry &registry = loadedVoxels();

	const spk::VoxelTypeId type = registry.typeId("bush");
	const pg::VoxelDefinition &definition = registry.get("bush");
	EXPECT_EQ(definition.typeId, type);
	EXPECT_EQ(definition.id, "bush");
	EXPECT_EQ(&registry.get(type), &definition);

	// Existing single-state voxels stay compatible: state 0, name "default".
	ASSERT_EQ(definition.states.size(), 1u);
	EXPECT_EQ(definition.states.front().id, spk::VoxelStateId{0});
	EXPECT_EQ(definition.states.front().name, "default");
	EXPECT_EQ(registry.runtimeId("bush"), definition.defaultState().runtimeId);
	EXPECT_EQ(registry.numericId("bush"), definition.defaultState().runtimeId);
}

TEST(VoxelRegistryStates, RuntimeIdResolvesBackToDefinitionAndState)
{
	const pg::VoxelRegistry &registry = loadedVoxels();

	const spk::VoxelRuntimeId runtime = registry.runtimeId("stone-block");
	EXPECT_EQ(registry.definition(runtime).id, "stone-block");
	EXPECT_EQ(registry.state(runtime).runtimeId, runtime);
	EXPECT_EQ(registry.state(runtime).id, spk::VoxelStateId{0});
}

TEST(VoxelRegistryStates, TypeCountDiffersFromRuntimeStateCount)
{
	const pg::VoxelRegistry &registry = loadedVoxels();

	// water contributes its generated fill stages, so there must be strictly more
	// runtime states than semantic types, and exactly one string id per type.
	EXPECT_GT(registry.runtimeStateCount(), registry.typeCount());
	EXPECT_EQ(registry.ids().size(), registry.typeCount());
	EXPECT_EQ(registry.renderRegistry().typeCount(), registry.typeCount());
	EXPECT_EQ(registry.renderRegistry().runtimeStateCount(), registry.runtimeStateCount());

	// No synthetic per-state string ids ("water#1") leak into the semantic catalog.
	for (const std::string &id : registry.ids())
	{
		EXPECT_EQ(id.find('#'), std::string::npos) << id;
	}
}

TEST(VoxelRegistryStates, MultiStateVoxelResolvesExplicitStates)
{
	VoxelDirectory directory;
	directory.add("two-state-bush", kTwoStateBush);
	pg::VoxelRegistry registry;
	registry.load(loadedShapes(), directory.path());

	EXPECT_EQ(registry.typeCount(), 1u);
	EXPECT_EQ(registry.runtimeStateCount(), 2u);

	const pg::VoxelDefinition &definition = registry.get("two-state-bush");
	ASSERT_EQ(definition.states.size(), 2u);
	const spk::VoxelRuntimeId defaultRuntime = registry.runtimeId("two-state-bush");
	const spk::VoxelRuntimeId alternateRuntime = registry.runtimeId("two-state-bush", spk::VoxelStateId{1});
	EXPECT_NE(defaultRuntime, alternateRuntime);
	EXPECT_EQ(defaultRuntime, definition.state(spk::VoxelStateId{0}).runtimeId);
	EXPECT_EQ(alternateRuntime, definition.state(spk::VoxelStateId{1}).runtimeId);

	// Both states resolve to the same semantic definition but their own state entry.
	EXPECT_EQ(&registry.definition(alternateRuntime), &definition);
	EXPECT_EQ(registry.state(alternateRuntime).name, "alternate");
	EXPECT_EQ(registry.debugName(defaultRuntime), "two-state-bush");
	EXPECT_EQ(registry.debugName(alternateRuntime), "two-state-bush@alternate");

	// The two states render with different textures through their own runtime ids.
	spk::VoxelGrid grid({1, 1, 1});
	grid.cell(0, 0, 0) = {defaultRuntime};
	const spk::VoxelRenderMeshes defaultMesh = spk::VoxelMesher::buildRenderMesh(grid, registry.renderRegistry());
	grid.cell(0, 0, 0) = {alternateRuntime};
	const spk::VoxelRenderMeshes alternateMesh = spk::VoxelMesher::buildRenderMesh(grid, registry.renderRegistry());
	ASSERT_EQ(defaultMesh.opaque.vertices().size(), alternateMesh.opaque.vertices().size());
	bool foundUvDifference = false;
	for (std::size_t index = 0; index < defaultMesh.opaque.vertices().size(); ++index)
	{
		foundUvDifference = foundUvDifference ||
							defaultMesh.opaque.vertices()[index].uv != alternateMesh.opaque.vertices()[index].uv;
	}
	EXPECT_TRUE(foundUvDifference);

	// Unknown states are rejected.
	EXPECT_THROW((void)registry.runtimeId("two-state-bush", spk::VoxelStateId{2}), std::out_of_range);
}

// ---------------------------------------------------------------------------
// Fluid compatibility: one semantic type, source + levels as states
// ---------------------------------------------------------------------------

TEST(VoxelRegistryFluids, WaterSourceAndLevelsShareOneType)
{
	const pg::VoxelRegistry &registry = loadedVoxels();

	ASSERT_FALSE(registry.fluids().empty());
	const pg::FluidFamily &family = registry.fluids().front();
	EXPECT_EQ(family.type, registry.typeId("water"));

	// Source is state zero, flow levels use states one through N.
	EXPECT_EQ(family.source.state, spk::VoxelStateId{0});
	EXPECT_TRUE(family.source.source);
	EXPECT_EQ(family.source.runtime, registry.runtimeId("water"));
	ASSERT_EQ(family.levels.size(), static_cast<std::size_t>(family.maxSpread));
	std::vector<spk::VoxelRuntimeId> runtimes{family.source.runtime};
	for (int stage = 1; stage <= family.maxSpread; ++stage)
	{
		const pg::FluidStateRef &level = family.levels[static_cast<std::size_t>(stage - 1)];
		EXPECT_EQ(level.type, family.type);
		EXPECT_EQ(level.state, spk::VoxelStateId{static_cast<std::uint16_t>(stage)});
		EXPECT_EQ(level.level, static_cast<std::size_t>(stage));
		EXPECT_FALSE(level.source);
		EXPECT_EQ(level.runtime, registry.runtimeId("water", level.state));
		runtimes.push_back(level.runtime);
	}

	// Each state has a distinct runtime id.
	for (std::size_t left = 0; left < runtimes.size(); ++left)
	{
		for (std::size_t right = left + 1; right < runtimes.size(); ++right)
		{
			EXPECT_NE(runtimes[left], runtimes[right]);
		}
	}

	// The whole family belongs to the "water" definition (no separate semantic types).
	const pg::VoxelDefinition &water = registry.get("water");
	EXPECT_EQ(water.states.size(), static_cast<std::size_t>(family.maxSpread) + 1u);
	for (const spk::VoxelRuntimeId runtime : runtimes)
	{
		EXPECT_EQ(&registry.definition(runtime), &water);
	}
}

TEST(VoxelRegistryFluids, FluidRefsClassifySourceAndLevels)
{
	const pg::VoxelRegistry &registry = loadedVoxels();
	const pg::FluidFamily &family = registry.fluids().front();

	const pg::FluidRef *source = registry.tryFluidRef(family.source.runtime);
	ASSERT_NE(source, nullptr);
	EXPECT_TRUE(source->source);
	EXPECT_EQ(source->stage, family.maxSpread);

	for (int stage = 1; stage <= family.maxSpread; ++stage)
	{
		const pg::FluidRef *reference = registry.tryFluidRef(family.stageId(stage));
		ASSERT_NE(reference, nullptr);
		EXPECT_FALSE(reference->source);
		EXPECT_EQ(reference->stage, stage);
		EXPECT_EQ(reference->family, 0u);
	}

	// Ordinary voxels report no fluid classification.
	EXPECT_EQ(registry.tryFluidRef(registry.runtimeId("stone-block")), nullptr);

	// Debug names expose the state, not a synthetic voxel id.
	EXPECT_EQ(registry.debugName(family.source.runtime), "water@source");
	EXPECT_EQ(registry.debugName(family.stageId(1)), "water@1");
}
