#include "abilities/effect_describe.hpp"
#include "core/json.hpp"
#include "tools/core/content_document.hpp"
#include "tools/core/feat_board_document.hpp"
#include "tools/core/i_editor_page.hpp"
#include "tools/core/json_writer.hpp"
#include "tools/pages/voxel_modeler_page.hpp"
#include "tools/pages/world_editor_page.hpp"
#include "tools/widgets/graph_canvas.hpp"
#include "tools/widgets/property_panel.hpp"
#include "world/map_definition.hpp"
#include "world/prefab_definition.hpp"

#include <chrono>
#include <filesystem>
#include <string>

#include <gtest/gtest.h>

TEST(JsonWriter, RoundTripsEveryAuthoredDataSampleAndIsStable)
{
	const std::filesystem::path dataDirectory = std::filesystem::path(PG_RESOURCE_DIR) / "data";
	pg::tools::JsonWriter writer;
	std::size_t sampleCount = 0;
	for (const auto &entry : std::filesystem::recursive_directory_iterator(dataDirectory))
	{
		if (!entry.is_regular_file() || entry.path().extension() != ".json")
		{
			continue;
		}
		const nlohmann::json parsed = pg::JsonLoader::parseFile(entry.path());
		const std::string first = writer.serialize(parsed);
		const nlohmann::json reparsed = nlohmann::json::parse(first);
		EXPECT_EQ(parsed, reparsed) << entry.path();
		EXPECT_EQ(first, writer.serialize(reparsed)) << entry.path();
		++sampleCount;
	}
	EXPECT_GT(sampleCount, 0u);
}

TEST(JsonWriter, UsesSemanticTopLevelOrdering)
{
	pg::tools::JsonWriter writer;
	const nlohmann::json value = {
		{"shape", {{"textures", {{"side", {0, 0}}}}, {"type", "cube"}}},
		{"tags", nlohmann::json::array()},
		{"traversal", "solid"},
		{"version", 1}};
	const std::string serialized = writer.serialize(value);
	EXPECT_LT(serialized.find("\"version\""), serialized.find("\"traversal\""));
	EXPECT_LT(serialized.find("\"traversal\""), serialized.find("\"tags\""));
	EXPECT_LT(serialized.find("\"tags\""), serialized.find("\"shape\""));
	EXPECT_LT(serialized.find("\"type\""), serialized.find("\"textures\""));
}

TEST(EditorPageState, TracksCleanChangedAndSavedTransitions)
{
	pg::tools::EditorPageState state;
	EXPECT_FALSE(state.hasUnsavedChanges());
	state.markChanged();
	EXPECT_TRUE(state.hasUnsavedChanges());
	state.markSaved();
	EXPECT_FALSE(state.hasUnsavedChanges());
}

TEST(PropertyPanel, TypeSwapPreservesOnlyCompatibleCommonFields)
{
	const nlohmann::json current = {
		{"type", "slab"},
		{"height", 0.25},
		{"textures", {{"top", {2, 3}}, {"bottom", {4, 5}}, {"side", {6, 7}}}}};
	const nlohmann::json defaults = {
		{"type", "cube"},
		{"textures", {{"top", {0, 0}}, {"bottom", {0, 0}}, {"side", {0, 0}}}}};

	const nlohmann::json swapped = pg::tools::PropertyPanel::swapPolymorphicType(current, "cube", defaults);
	EXPECT_EQ(swapped.at("type"), "cube");
	EXPECT_FALSE(swapped.contains("height"));
	EXPECT_EQ(swapped.at("textures").at("top"), nlohmann::json({2, 3}));
	EXPECT_EQ(swapped.at("textures").at("side"), nlohmann::json({6, 7}));
}

TEST(VoxelModelDocument, NewDefinitionCanBeRenamedAndSavedAsItsRegistryId)
{
	const std::filesystem::path directory = std::filesystem::temp_directory_path() /
											("erelia-document-test-" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
	std::filesystem::create_directories(directory);
	struct Cleanup
	{
		std::filesystem::path path;
		~Cleanup()
		{
			std::error_code error;
			std::filesystem::remove_all(path, error);
		}
	} cleanup{directory};

	pg::tools::VoxelModelDocument document;
	document.load(directory);
	document.create("new-voxel", {{"version", 1}, {"traversal", "passable"}, {"tags", nlohmann::json::array()}, {"shape", {{"type", "crossPlane"}, {"textures", {{"plane", {0, 0}}}}}}});
	document.renameSelected("fence");
	ASSERT_TRUE(document.dirty());
	document.save(pg::tools::JsonWriter{});

	EXPECT_TRUE(std::filesystem::exists(directory / "fence.json"));
	EXPECT_FALSE(std::filesystem::exists(directory / "new-voxel.json"));
	EXPECT_EQ(pg::JsonLoader::parseFile(directory / "fence.json").at("shape").at("type"), "crossPlane");
}

TEST(WorldDocumentMath, BoxFillIncludesBothCornersInStableXYZOrder)
{
	const std::vector<spk::Vector3Int> cells = pg::tools::WorldDocument::boxCells({2, 3, 4}, {1, 2, 5});
	ASSERT_EQ(cells.size(), 8u);
	EXPECT_EQ(cells.front(), spk::Vector3Int(1, 2, 4));
	EXPECT_EQ(cells.back(), spk::Vector3Int(2, 3, 5));
}

TEST(WorldDocumentMath, PlaceAdjacentUsesTheDdaEntryFace)
{
	using Plane = pg::VoxelAxisPlane;
	EXPECT_EQ(
		pg::tools::WorldDocument::adjacentCell({{4, 5, 6}, Plane::PositiveX, 1.0f}),
		spk::Vector3Int(5, 5, 6));
	EXPECT_EQ(
		pg::tools::WorldDocument::adjacentCell({{4, 5, 6}, Plane::NegativeY, 1.0f}),
		spk::Vector3Int(4, 4, 6));
	EXPECT_FALSE(pg::tools::WorldDocument::adjacentCell({{4, 5, 6}, Plane::Count, 0.0f}).has_value());
}

TEST(WorldDocument, MapAndPrefabJsonRetainAuthoredStructuredContent)
{
	pg::Registries registries;
	registries.loadAll(std::filesystem::path(PG_RESOURCE_DIR) / "data");
	pg::tools::JsonWriter writer;
	const auto verify = [&](const std::filesystem::path &p_file, pg::tools::WorldDocument::Kind p_kind) {
		pg::tools::WorldDocument document;
		document.load(p_file, p_kind, registries);
		const nlohmann::json roundTripped = nlohmann::json::parse(writer.serialize(document.json()));
		EXPECT_EQ(roundTripped, pg::JsonLoader::parseFile(p_file));
	};
	verify(
		std::filesystem::path(PG_RESOURCE_DIR) / "data" / "maps" / "m1-testground.json",
		pg::tools::WorldDocument::Kind::Map);
	verify(
		std::filesystem::path(PG_RESOURCE_DIR) / "data" / "prefabs" / "small-house.json",
		pg::tools::WorldDocument::Kind::Prefab);
}

TEST(WorldDocument, AuthoredMapAndPrefabRemainParserCompatible)
{
	pg::Registries registries;
	registries.loadAll(std::filesystem::path(PG_RESOURCE_DIR) / "data");
	const std::filesystem::path directory = std::filesystem::temp_directory_path() /
											("erelia-world-document-test-" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
	std::filesystem::create_directories(directory);
	struct Cleanup
	{
		std::filesystem::path path;
		~Cleanup()
		{
			std::error_code error;
			std::filesystem::remove_all(path, error);
		}
	} cleanup{directory};

	pg::tools::WorldDocument mapDocument;
	mapDocument.create(directory, "test-map", pg::tools::WorldDocument::Kind::Map, registries.voxels(), {16, 8, 16});
	const pg::VoxelCell stone{registries.voxels().numericId("stone-block")};
	mapDocument.fillBox({0, 0, 0}, {3, 0, 3}, stone);
	mapDocument.addStamp("small-house", {4, 1, 4}, pg::VoxelOrientation::PositiveZ, registries.prefabs().get("small-house"));
	mapDocument.editJson()["markers"].push_back({{"name", "playerSpawn"}, {"at", {1, 1, 1}}});
	mapDocument.save(pg::tools::JsonWriter{});
	nlohmann::json mapJson = pg::JsonLoader::parseFile(directory / "test-map.json");
	pg::JsonReader mapReader(mapJson, directory / "test-map.json");
	const pg::MapDefinition map = pg::parseMapDefinition(mapReader, registries.voxels(), registries.prefabs());
	EXPECT_EQ(map.stamps.size(), 1u);
	EXPECT_EQ(map.markers.size(), 1u);
	EXPECT_EQ(map.grid.cell(0, 0, 0).id, stone.id);

	pg::tools::WorldDocument prefabDocument;
	prefabDocument.create(directory, "test-prefab", pg::tools::WorldDocument::Kind::Prefab, registries.voxels(), {3, 2, 3});
	prefabDocument.setCell({1, 0, 1}, stone);
	prefabDocument.editJson()["anchors"].push_back({{"name", "center"}, {"at", {1, 0, 1}}});
	prefabDocument.save(pg::tools::JsonWriter{});
	nlohmann::json prefabJson = pg::JsonLoader::parseFile(directory / "test-prefab.json");
	pg::JsonReader prefabReader(prefabJson, directory / "test-prefab.json");
	const pg::PrefabDefinition prefab = pg::parsePrefabDefinition(prefabReader, registries.voxels());
	EXPECT_EQ(prefab.anchors.size(), 1u);
	EXPECT_EQ(prefab.grid.cell(1, 0, 1).id, stone.id);
}

TEST(GraphCanvas, CanvasScreenTransformsRoundTripAndHitTestingUsesZoomPan)
{
	pg::tools::GraphCanvas canvas("Graph");
	canvas.setGeometry({0, 0, 800, 600});
	canvas.setGraph({{.id = "node", .label = "Node", .position = {1.5f, -0.5f}}}, {});
	canvas.frameAll();
	const spk::Vector2 screen = canvas.canvasToScreen({1.5f, -0.5f});
	const spk::Vector2 roundTrip = canvas.screenToCanvas(screen);
	EXPECT_NEAR(roundTrip.x, 1.5f, 0.0001f);
	EXPECT_NEAR(roundTrip.y, -0.5f, 0.0001f);
	EXPECT_EQ(canvas.hitTest({static_cast<int>(screen.x), static_cast<int>(screen.y)}), "node");
}

TEST(FeatBoardDocument, RequirementTypeSwapPreservesItsStableUuid)
{
	pg::tools::FeatBoardDocument document;
	const std::filesystem::path source = std::filesystem::path(PG_RESOURCE_DIR) / "data" / "featboards" / "sprout-board.json";
	document.load(source);
	document.selectNode(document.json().at("nodes").at(1).at("uuid").get<std::string>());
	const std::string uuid = document.selectedNodeJson()->at("requirements").at(0).at("uuid").get<std::string>();
	document.setRequirementType(0, "healHealth");
	EXPECT_EQ(document.selectedNodeJson()->at("requirements").at(0).at("uuid"), uuid);
	EXPECT_EQ(document.selectedNodeJson()->at("requirements").at(0).at("type"), "healHealth");
}

TEST(FeatBoardValidation, DetectsAndRepairsAsymmetricNeighbours)
{
	pg::Registries registries;
	registries.loadAll(std::filesystem::path(PG_RESOURCE_DIR) / "data");
	nlohmann::json json = pg::JsonLoader::parseFile(
		std::filesystem::path(PG_RESOURCE_DIR) / "data" / "featboards" / "sprout-board.json");
	const std::string root = json.at("nodes").at(0).at("uuid").get<std::string>();
	json["nodes"][1]["neighbours"].erase(json["nodes"][1]["neighbours"].begin());
	const auto before = pg::tools::FeatBoardDocument::validate(json, "sprout-board", registries);
	EXPECT_TRUE(std::ranges::any_of(before, [](const auto &p_issue) {
		return p_issue.code == pg::tools::FeatBoardValidationIssue::Code::AsymmetricNeighbour;
	}));
	EXPECT_EQ(pg::tools::FeatBoardDocument::fixNeighbourSymmetry(json), 1u);
	const auto after = pg::tools::FeatBoardDocument::validate(json, "sprout-board", registries);
	EXPECT_FALSE(std::ranges::any_of(after, [](const auto &p_issue) {
		return p_issue.code == pg::tools::FeatBoardValidationIssue::Code::AsymmetricNeighbour;
	}));
	EXPECT_TRUE(std::ranges::any_of(json["nodes"][1]["neighbours"], [&root](const auto &p_value) {
		return p_value.is_string() && p_value.template get<std::string>() == root;
	}));
}

TEST(FeatBoardValidation, DetectsDuplicateDisconnectedAndDanglingIds)
{
	pg::Registries registries;
	registries.loadAll(std::filesystem::path(PG_RESOURCE_DIR) / "data");
	const auto source = std::filesystem::path(PG_RESOURCE_DIR) / "data" / "featboards" / "sprout-board.json";
	nlohmann::json duplicate = pg::JsonLoader::parseFile(source);
	duplicate["nodes"][1]["uuid"] = duplicate["nodes"][0]["uuid"];
	auto issues = pg::tools::FeatBoardDocument::validate(duplicate, "sprout-board", registries);
	EXPECT_TRUE(std::ranges::any_of(issues, [](const auto &p_issue) {
		return p_issue.code == pg::tools::FeatBoardValidationIssue::Code::DuplicateUuid;
	}));

	nlohmann::json disconnected = pg::JsonLoader::parseFile(source);
	const std::string isolated = disconnected["nodes"].back()["uuid"];
	for (auto &node : disconnected["nodes"])
	{
		for (auto iterator = node["neighbours"].begin(); iterator != node["neighbours"].end();)
		{
			if (iterator->is_string() && iterator->get<std::string>() == isolated)
			{
				iterator = node["neighbours"].erase(iterator);
			}
			else
			{
				++iterator;
			}
		}
	}
	disconnected["nodes"].back()["neighbours"] = nlohmann::json::array();
	issues = pg::tools::FeatBoardDocument::validate(disconnected, "sprout-board", registries);
	EXPECT_TRUE(std::ranges::any_of(issues, [](const auto &p_issue) {
		return p_issue.code == pg::tools::FeatBoardValidationIssue::Code::DisconnectedGraph;
	}));

	nlohmann::json dangling = pg::JsonLoader::parseFile(source);
	dangling["nodes"][2]["rewards"][0]["ability"] = "missing-ability";
	issues = pg::tools::FeatBoardDocument::validate(dangling, "sprout-board", registries);
	EXPECT_TRUE(std::ranges::any_of(issues, [](const auto &p_issue) {
		return p_issue.code == pg::tools::FeatBoardValidationIssue::Code::DanglingAbility;
	}));
}

TEST(FeatBoardValidation, ReordersFormNodesByLinkedSpeciesTier)
{
	pg::Registries registries;
	registries.loadAll(std::filesystem::path(PG_RESOURCE_DIR) / "data");
	nlohmann::json json = pg::JsonLoader::parseFile(
		std::filesystem::path(PG_RESOURCE_DIR) / "data" / "featboards" / "sprout-board.json");
	const std::string root = json["rootNode"];
	const std::string uuid = spk::UUID::generate().toString();
	json["nodes"].push_back({{"uuid", uuid}, {"displayName", "Return to Base"}, {"position", {5, 1}}, {"kind", "form"}, {"neighbours", {root}}, {"repeatLimit", 1}, {"requirements", nlohmann::json::array()}, {"rewards", {{{"type", "changeForm"}, {"form", "base"}}}}});
	json["nodes"][0]["neighbours"].push_back(uuid);
	const auto issues = pg::tools::FeatBoardDocument::validate(json, "sprout-board", registries);
	EXPECT_TRUE(std::ranges::any_of(issues, [](const auto &p_issue) {
		return p_issue.code == pg::tools::FeatBoardValidationIssue::Code::FormTierOrder;
	}));
	EXPECT_TRUE(pg::tools::FeatBoardDocument::reorderFormNodes(json, "sprout-board", registries));
	std::vector<std::string> forms;
	for (const auto &node : json["nodes"])
	{
		if (node.value("kind", "") == "form")
		{
			forms.push_back(node["rewards"][0]["form"]);
		}
	}
	ASSERT_EQ(forms.size(), 2u);
	EXPECT_EQ(forms[0], "base");
	EXPECT_EQ(forms[1], "blaze");
}

TEST(FeatBoardDocument, FieldEditAndWriterRoundTripPreserveEveryUuid)
{
	const std::filesystem::path directory = std::filesystem::temp_directory_path() /
											("erelia-featboard-test-" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
	std::filesystem::create_directories(directory);
	struct Cleanup
	{
		std::filesystem::path path;
		~Cleanup()
		{
			std::error_code error;
			std::filesystem::remove_all(path, error);
		}
	} cleanup{directory};
	const std::filesystem::path source = std::filesystem::path(PG_RESOURCE_DIR) / "data" / "featboards" / "sprout-board.json";
	const std::filesystem::path target = directory / "sprout-board.json";
	pg::tools::JsonWriter writer;
	writer.write(target, pg::JsonLoader::parseFile(source));

	pg::tools::FeatBoardDocument document;
	document.load(target);
	std::vector<std::string> before;
	for (const auto &node : document.json()["nodes"])
	{
		before.push_back(node["uuid"]);
		for (const auto &requirement : node["requirements"])
		{
			before.push_back(requirement["uuid"]);
		}
	}
	document.editJson()["nodes"][1]["displayName"] = "Edited Name";
	document.save(writer);

	const nlohmann::json saved = pg::JsonLoader::parseFile(target);
	std::vector<std::string> after;
	for (const auto &node : saved["nodes"])
	{
		after.push_back(node["uuid"]);
		for (const auto &requirement : node["requirements"])
		{
			after.push_back(requirement["uuid"]);
		}
	}
	EXPECT_EQ(after, before);
}

TEST(ContentDocument, AuthoredStep28DomainsRoundTripAndValidate)
{
	pg::Registries registries;
	registries.loadAll(std::filesystem::path(PG_RESOURCE_DIR) / "data");
	const std::array samples = {
		std::pair{pg::tools::ContentDocument::Domain::Encounter, "encounter-tables/forest-basic.json"},
		std::pair{pg::tools::ContentDocument::Domain::Ability, "abilities/ember.json"},
		std::pair{pg::tools::ContentDocument::Domain::Status, "statuses/burn.json"},
		std::pair{pg::tools::ContentDocument::Domain::Species, "creatures/sprout.json"}};
	pg::tools::JsonWriter writer;
	for (const auto &[domain, relative] : samples)
	{
		const auto file = std::filesystem::path(PG_RESOURCE_DIR) / "data" / relative;
		pg::tools::ContentDocument document;
		document.load(file, domain);
		EXPECT_TRUE(document.validate(registries).empty()) << relative;
		EXPECT_EQ(nlohmann::json::parse(writer.serialize(document.json())), pg::JsonLoader::parseFile(file));
	}
}

TEST(ContentDocument, RulesPreviewUsesTheSharedDescribeFunction)
{
	pg::Registries registries;
	registries.loadAll(std::filesystem::path(PG_RESOURCE_DIR) / "data");
	const auto file = std::filesystem::path(PG_RESOURCE_DIR) / "data" / "abilities" / "ember.json";
	pg::tools::ContentDocument document;
	document.load(file, pg::tools::ContentDocument::Domain::Ability);
	nlohmann::json json = pg::JsonLoader::parseFile(file);
	pg::JsonReader reader(json, file);
	const pg::Ability ability = pg::parseAbility(reader, registries.statuses());
	std::string expected;
	for (const auto &effect : ability.effects)
	{
		expected += pg::describe(*effect) + "\n";
	}
	EXPECT_EQ(document.rulesPreview(registries), expected);
}

TEST(ContentDocument, CompletedNodeSummaryAppliesProgressRewards)
{
	pg::Registries registries;
	registries.loadAll(std::filesystem::path(PG_RESOURCE_DIR) / "data");
	const std::string summary = pg::tools::ContentDocument::completedNodesSummary(
		"sprout", nlohmann::json::array({"10000000-0000-4000-8000-000000000002"}), registries);
	EXPECT_NE(summary.find("HP 14"), std::string::npos);
	EXPECT_NE(summary.find("base"), std::string::npos);
}

TEST(ContentDocument, EveryEffectDefaultParsesAndDescribes)
{
	for (const std::string &type : pg::tools::ContentDocument::effectTypes())
	{
		nlohmann::json json = pg::tools::ContentDocument::effectDefaults(type);
		pg::JsonReader reader(json, "effect-default.json");
		const std::unique_ptr<pg::Effect> effect = pg::parseEffect(reader);
		EXPECT_FALSE(pg::describe(*effect).empty()) << type;
	}
}
