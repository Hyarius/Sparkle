#include <gtest/gtest.h>

#include "core/paths.hpp"
#include "voxel/shape_catalog.hpp"
#include "voxel/voxel_definition.hpp"
#include "voxel/voxel_family_definition.hpp"
#include "voxel/voxel_registry.hpp"

#include "structures/voxel/spk_voxel_mesher.hpp"

#include <cstdint>
#include <filesystem>
#include <fstream>
#include <string>
#include <string_view>
#include <variant>
#include <vector>

namespace
{
	[[nodiscard]] const pg::ShapeCatalog &loadedShapes()
	{
		static const pg::ShapeCatalog shapes = [] {
			pg::ShapeCatalog result;
			spk::loadJsonDirectory(
				result,
				pg::resourceRoot() / "data" / "shapes",
				[](std::string_view p_id, pg::JsonReader &p_reader) {
					pg::ShapeDefinition definition = pg::parseShapeDefinition(p_reader);
					definition.id = p_id;
					return definition;
				});
			return result;
		}();
		return shapes;
	}

	[[nodiscard]] const pg::VoxelFamilyCatalog &loadedFamilies()
	{
		static const pg::VoxelFamilyCatalog families = [] {
			pg::VoxelFamilyCatalog result;
			spk::loadJsonDirectory(
				result,
				pg::resourceRoot() / "data" / "voxel_families",
				[](std::string_view p_id, pg::JsonReader &p_reader) {
					pg::VoxelFamilyDefinition definition = pg::parseVoxelFamilyDefinition(p_reader, loadedShapes());
					definition.id = p_id;
					return definition;
				});
			return result;
		}();
		return families;
	}

	// The full resource catalog, loaded once: also proves every shipped voxel file still
	// parses under the type/state model.
	[[nodiscard]] const pg::VoxelRegistry &loadedVoxels()
	{
		static const pg::VoxelRegistry registry = [] {
			pg::VoxelRegistry result;
			result.load(loadedShapes(), loadedFamilies(), pg::resourceRoot() / "data" / "voxels");
			return result;
		}();
		return registry;
	}

	[[nodiscard]] pg::ParsedVoxel parseFromText(const std::string &p_json)
	{
		const spk::JSON::Value value = spk::JSON::Value::fromString(p_json);
		pg::JsonReader reader(value, "<test>");
		return pg::parseVoxelDefinition(reader, loadedShapes(), loadedFamilies());
	}

	TEST(VoxelLightingDefinition, ParsesPointLightWithSparkleColor)
	{
		const pg::ParsedVoxel voxel = parseFromText(R"({
        "version": 2, "traversal": "solid", "tags": [], "shape": "cube",
        "light": { "type": "point", "color": "#3366CC80", "power": 8.0, "reach": 12.0 },
        "textures": { "top": [0,0], "bottom": [0,0], "side": [0,0] }
    })");
		ASSERT_TRUE(voxel.light.has_value());
		EXPECT_EQ(voxel.light->type, pg::VoxelLightType::Point);
		EXPECT_FLOAT_EQ(voxel.light->color.r, 0x33 / 255.0f);
		EXPECT_FLOAT_EQ(voxel.light->color.a, 0x80 / 255.0f);
		EXPECT_FLOAT_EQ(voxel.light->power, 8.0f);
		EXPECT_FLOAT_EQ(voxel.light->reach, 12.0f);
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

	ASSERT_TRUE(std::holds_alternative<pg::ParsedRegularVoxel>(voxel.rendering));
	const pg::ParsedRegularVoxel &regular = std::get<pg::ParsedRegularVoxel>(voxel.rendering);
	ASSERT_EQ(regular.states.size(), 1u);
	EXPECT_EQ(regular.states.front().id, spk::VoxelStateId{0});
	EXPECT_EQ(regular.states.front().name, "default");
	EXPECT_NE(regular.states.front().shape, nullptr);
	EXPECT_EQ(voxel.data.traversal, pg::VoxelTraversal::Passable);
}

TEST(VoxelParsing, MovementCostDefaultsToOneAndIsSharedByTheType)
{
	// A voxel that authors no movement cost walks as free ground: the optional field defaults to 1,
	// so every voxel file written before battle movement existed stays valid.
	const pg::ParsedVoxel plain = parseFromText(R"({
		"version": 2, "traversal": "solid", "tags": ["ground"], "shape": "cube",
		"textures": {"top": [0, 0], "bottom": [0, 0], "side": [0, 0]}
	})");
	EXPECT_EQ(plain.data.movementCost, 1);

	// When present it is authored data on the semantic type, shared by every state: mud costs 2.
	const pg::ParsedVoxel mud = parseFromText(R"({
		"version": 2, "traversal": "solid", "tags": ["ground"], "shape": "cube", "movementCost": 2,
		"textures": {"top": [0, 0], "bottom": [0, 0], "side": [0, 0]}
	})");
	EXPECT_EQ(mud.data.movementCost, 2);
}

TEST(VoxelParsing, MovementCostRejectsZeroNegativeExcessiveAndNonInteger)
{
	const auto costed = [](std::string_view p_cost) {
		return std::string(R"({"version": 2, "traversal": "solid", "tags": ["ground"], "shape": "cube", "movementCost": )") +
			   std::string(p_cost) + R"(, "textures": {"top": [0, 0], "bottom": [0, 0], "side": [0, 0]}})";
	};

	EXPECT_NO_THROW((void)parseFromText(costed("1")));
	EXPECT_NO_THROW((void)parseFromText(costed("1000000")));
	// A 0 or a negative cost would make a cell free or paradoxical; over the ceiling is absurd; a
	// fractional cost is not a movement point count. Each fails with its field, not a silent clamp.
	EXPECT_THROW((void)parseFromText(costed("0")), pg::JsonError);
	EXPECT_THROW((void)parseFromText(costed("-1")), pg::JsonError);
	EXPECT_THROW((void)parseFromText(costed("1000001")), pg::JsonError);
	EXPECT_THROW((void)parseFromText(costed("1.5")), pg::JsonError);
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
			"traversal": "passable",
			"tags": [],
			"textures": { "top": [0, 0], "bottom": [2, 0], "side": [1, 0] },
			"fluid": { "maxSpread": 4 },
			"states": [ { "id": 0, "textures": { "top": [0, 0], "bottom": [2, 0], "side": [1, 0] } } ]
		})"),
		pg::JsonError);
}

// ---------------------------------------------------------------------------
// Parsing: version 3 fluids (generated states)
// ---------------------------------------------------------------------------

namespace
{
	constexpr const char *kFluidJson = R"({
		"version": 3,
		"traversal": "passable",
		"tags": ["water", "river", "losTransparent"],
		"transparency": 0.5,
		"textures": { "top": [2, 1], "bottom": [3, 1], "side": [4, 1] },
		"fluid": { "maxSpread": 6, "falls": false }
	})";
}

TEST(VoxelParsing, FluidBlockYieldsAGeneratedFluidDescription)
{
	const pg::ParsedVoxel voxel = parseFromText(kFluidJson);

	ASSERT_TRUE(std::holds_alternative<pg::ParsedFluidVoxel>(voxel.rendering));
	const spk::VoxelFluidDescription &description = std::get<pg::ParsedFluidVoxel>(voxel.rendering).description;
	EXPECT_EQ(description.levelCount, 6u);
	EXPECT_FALSE(description.falls);
	EXPECT_FLOAT_EQ(description.appearance.transparency, 0.5f);
	EXPECT_EQ(description.appearance.topTexture, (spk::AtlasCell{2, 1}));
	EXPECT_EQ(description.appearance.bottomTexture, (spk::AtlasCell{3, 1}));
	EXPECT_EQ(description.appearance.sideTexture, (spk::AtlasCell{4, 1}));
	EXPECT_EQ(voxel.data.traversal, pg::VoxelTraversal::Passable);
	EXPECT_TRUE(voxel.data.hasTag("water"));
}

TEST(VoxelParsing, FluidFallsDefaultsToTrueAndSpreadDefaultsToEight)
{
	const pg::ParsedVoxel voxel = parseFromText(R"({
		"version": 3,
		"traversal": "passable",
		"tags": [],
		"textures": { "top": [2, 1], "bottom": [2, 1], "side": [2, 1] },
		"fluid": {}
	})");

	const spk::VoxelFluidDescription &description = std::get<pg::ParsedFluidVoxel>(voxel.rendering).description;
	EXPECT_TRUE(description.falls);
	EXPECT_EQ(description.levelCount, 8u);
}

TEST(VoxelParsing, FluidRequiresVersionThree)
{
	EXPECT_THROW(
		(void)parseFromText(R"({
			"version": 2,
			"traversal": "passable",
			"tags": [],
			"shape": "cube",
			"textures": { "top": [2, 1], "bottom": [2, 1], "side": [2, 1] },
			"fluid": { "maxSpread": 8 }
		})"),
		pg::JsonError);
}

TEST(VoxelParsing, FluidForbidsShape)
{
	EXPECT_THROW(
		(void)parseFromText(R"({
			"version": 3,
			"traversal": "passable",
			"tags": [],
			"shape": "cube",
			"textures": { "top": [2, 1], "bottom": [2, 1], "side": [2, 1] },
			"fluid": { "maxSpread": 8 }
		})"),
		pg::JsonError);
}

TEST(VoxelParsing, FluidRejectsMissingTextureSlot)
{
	EXPECT_THROW(
		(void)parseFromText(R"({
			"version": 3,
			"traversal": "passable",
			"tags": [],
			"textures": { "top": [2, 1], "bottom": [2, 1] },
			"fluid": { "maxSpread": 8 }
		})"),
		pg::JsonError);
}

TEST(VoxelParsing, FluidRejectsExtraTextureSlot)
{
	EXPECT_THROW(
		(void)parseFromText(R"({
			"version": 3,
			"traversal": "passable",
			"tags": [],
			"textures": { "top": [2, 1], "bottom": [2, 1], "side": [2, 1], "extra": [0, 0] },
			"fluid": { "maxSpread": 8 }
		})"),
		pg::JsonError);
}

TEST(VoxelParsing, FluidRejectsInvalidSpread)
{
	for (const char *spread : {"0", "17", "-3"})
	{
		EXPECT_THROW(
			(void)parseFromText(std::string(R"({
				"version": 3,
				"traversal": "passable",
				"tags": [],
				"textures": { "top": [2, 1], "bottom": [2, 1], "side": [2, 1] },
				"fluid": { "maxSpread": )") + spread + "} }"),
			pg::JsonError)
			<< spread;
	}
}

TEST(VoxelParsing, FluidRejectsSolidTraversal)
{
	EXPECT_THROW(
		(void)parseFromText(R"({
			"version": 3,
			"traversal": "solid",
			"tags": [],
			"textures": { "top": [2, 1], "bottom": [2, 1], "side": [2, 1] },
			"fluid": { "maxSpread": 8 }
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

	ASSERT_TRUE(std::holds_alternative<pg::ParsedRegularVoxel>(voxel.rendering));
	const pg::ParsedRegularVoxel &regular = std::get<pg::ParsedRegularVoxel>(voxel.rendering);
	ASSERT_EQ(regular.states.size(), 2u);
	const pg::ShapeDefinition &cube = loadedShapes().get("cube");
	for (const pg::ParsedVoxelState &state : regular.states)
	{
		EXPECT_NE(state.shape, nullptr);
		// Every ordinary state receives the shared shape's cardinal heights.
		EXPECT_EQ(state.heights.positiveY.stationary, cube.heights.positiveY.stationary);
		EXPECT_EQ(state.heights.positiveY.positiveX, cube.heights.positiveY.positiveX);
	}
	EXPECT_EQ(regular.states[0].name, "default");
	EXPECT_EQ(regular.states[1].name, "mossy");
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
	registry.load(loadedShapes(), loadedFamilies(), directory.path());

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

TEST(VoxelRegistryStates, PlaygroundFamilyExpandsOneMaterialIntoShapeStates)
{
	const pg::VoxelRegistry &registry = loadedVoxels();
	const pg::VoxelDefinition &grass = registry.get("grass-block");
	ASSERT_EQ(grass.states.size(), 4u);
	EXPECT_EQ(grass.states[0].name, "default");
	EXPECT_EQ(grass.states[1].name, "slab");
	EXPECT_EQ(grass.states[2].name, "stair");
	EXPECT_EQ(grass.states[3].name, "slope");
	EXPECT_FLOAT_EQ(grass.states[0].heights.positiveY.stationary, 1.0f);
	EXPECT_FLOAT_EQ(grass.states[1].heights.positiveY.stationary, 0.5f);
	EXPECT_EQ(registry.definition(grass.states[1].runtimeId).typeId, grass.typeId);

	const pg::VoxelDefinition &road = registry.get("forest-road");
	ASSERT_NE(road.tryState("slab"), nullptr);
	EXPECT_EQ(registry.definition(road.tryState("slab")->runtimeId).id, "forest-road");
}

// ---------------------------------------------------------------------------
// Fluids: one semantic type, Sparkle-generated source + level states
// ---------------------------------------------------------------------------

TEST(VoxelRegistryFluids, WaterSourceAndLevelsShareOneType)
{
	const pg::VoxelRegistry &registry = loadedVoxels();
	const spk::VoxelRegistry &render = registry.renderRegistry();

	ASSERT_TRUE(render.hasFluids());
	ASSERT_EQ(render.fluidFamilies().size(), 1u);
	const spk::VoxelFluidFamily &family = render.fluidFamilies().front();
	EXPECT_EQ(family.type, registry.typeId("water"));

	// Source is state zero, flow levels use states one through N.
	EXPECT_EQ(family.sourceState, spk::VoxelStateId{0});
	EXPECT_EQ(family.sourceRuntime, registry.runtimeId("water"));
	ASSERT_EQ(family.levelCount(), 8u);
	std::vector<spk::VoxelRuntimeId> runtimes{family.sourceRuntime};
	for (std::size_t level = 1; level <= family.levelCount(); ++level)
	{
		const spk::VoxelFluidState &state = family.level(level);
		EXPECT_EQ(state.state, (spk::VoxelStateId{static_cast<std::uint16_t>(level)}));
		EXPECT_EQ(state.level, level);
		EXPECT_EQ(state.runtime, registry.runtimeId("water", state.state));
		runtimes.push_back(state.runtime);
	}

	// Each state has a distinct runtime id.
	for (std::size_t left = 0; left < runtimes.size(); ++left)
	{
		for (std::size_t right = left + 1; right < runtimes.size(); ++right)
		{
			EXPECT_NE(runtimes[left], runtimes[right]);
		}
	}

	// The whole family belongs to the "water" definition (no separate semantic types and
	// no synthetic "water#k" string ids); type-level gameplay data is shared by every
	// state: all passable, all tagged.
	const pg::VoxelDefinition &water = registry.get("water");
	EXPECT_EQ(water.states.size(), family.levelCount() + 1u);
	EXPECT_EQ(water.data.traversal, pg::VoxelTraversal::Passable);
	EXPECT_TRUE(water.data.hasTag("water"));
	for (const spk::VoxelRuntimeId runtime : runtimes)
	{
		EXPECT_EQ(&registry.definition(runtime), &water);
	}
}

TEST(VoxelRegistryFluids, FluidRefsClassifySourceAndLevels)
{
	const pg::VoxelRegistry &registry = loadedVoxels();
	const spk::VoxelRegistry &render = registry.renderRegistry();
	const spk::VoxelFluidFamily &family = render.fluidFamilies().front();

	const spk::VoxelFluidRef *source = render.tryFluidRef(family.sourceRuntime);
	ASSERT_NE(source, nullptr);
	EXPECT_TRUE(source->source);
	EXPECT_EQ(source->level, family.levelCount());

	for (std::size_t level = 1; level <= family.levelCount(); ++level)
	{
		const spk::VoxelFluidRef *reference = render.tryFluidRef(family.level(level).runtime);
		ASSERT_NE(reference, nullptr);
		EXPECT_FALSE(reference->source);
		EXPECT_EQ(reference->level, level);
		EXPECT_EQ(reference->family, 0u);
	}

	// Ordinary voxels report no fluid classification.
	EXPECT_EQ(render.tryFluidRef(registry.runtimeId("stone-block")), nullptr);

	// Debug names expose the state, not a synthetic voxel id.
	EXPECT_EQ(registry.debugName(family.sourceRuntime), "water@source");
	EXPECT_EQ(registry.debugName(family.level(1).runtime), "water@1");
}

TEST(VoxelRegistryFluids, CardinalHeightsMatchTheRenderedFillHeights)
{
	const pg::VoxelRegistry &registry = loadedVoxels();
	const spk::VoxelFluidFamily &family = registry.renderRegistry().fluidFamilies().front();
	const pg::VoxelDefinition &water = registry.get("water");

	const auto expectFlat = [](const pg::CardinalHeightCollection &p_heights, float p_expected) {
		for (const pg::CardinalHeightSet *set : {&p_heights.positiveY, &p_heights.negativeY})
		{
			EXPECT_FLOAT_EQ(set->positiveX, p_expected);
			EXPECT_FLOAT_EQ(set->negativeX, p_expected);
			EXPECT_FLOAT_EQ(set->positiveZ, p_expected);
			EXPECT_FLOAT_EQ(set->negativeZ, p_expected);
			EXPECT_FLOAT_EQ(set->stationary, p_expected);
		}
	};

	// The source surface sits at full height, flow level k at k / N - the same heights
	// the generated render slabs use. Water stays non-walkable regardless (passable type).
	expectFlat(water.state(spk::VoxelStateId{0}).heights, 1.0f);
	for (std::size_t level = 1; level <= family.levelCount(); ++level)
	{
		expectFlat(
			water.state(spk::VoxelStateId{static_cast<std::uint16_t>(level)}).heights,
			family.level(level).height);
	}
}
