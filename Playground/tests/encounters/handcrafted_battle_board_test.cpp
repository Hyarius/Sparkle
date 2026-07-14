#include <gtest/gtest.h>

#include "board/handcrafted_battle_board_definition.hpp"
#include "support/json_fixture.hpp"

#include "structures/voxel/spk_prefab.hpp"

#include <string>
#include <string_view>

namespace
{
	// A prefab whose voxels are placed by hand, so a case can put one cell exactly where it wants
	// to prove the bounds check sees it.
	[[nodiscard]] pg::Registry<pg::PrefabDefinition> prefabs(
		const std::vector<spk::Vector3Int> &p_positions,
		const spk::Vector3Int &p_pivot = {0, 0, 0})
	{
		pg::PrefabDefinition definition;
		definition.id = "arena-geometry";
		for (const spk::Vector3Int &position : p_positions)
		{
			// An explicitly authored empty cell: it carves, so it is applied, so it is bounded.
			definition.prefab.addVoxel(position, spk::VoxelCell{});
		}
		definition.prefab.setPivot(p_pivot);

		pg::Registry<pg::PrefabDefinition> registry;
		registry.add("arena-geometry", std::move(definition));
		return registry;
	}

	[[nodiscard]] std::string board(
		std::string_view p_size = "[13, 3, 13]",
		std::string_view p_approach = "positiveZ",
		std::string_view p_depth = "2",
		std::string_view p_prefab = "arena-geometry")
	{
		return std::string(R"({"version": 1, "displayNameKey": "battle-board.fixture.name", "size": )") +
			   std::string(p_size) + R"(, "geometryPrefab": ")" + std::string(p_prefab) +
			   R"(", "playerApproach": ")" + std::string(p_approach) + R"(", "deploymentDepth": )" +
			   std::string(p_depth) + "}";
	}

	[[nodiscard]] pg::HandcraftedBattleBoardDefinition parse(
		const std::string &p_document,
		const pg::Registry<pg::PrefabDefinition> &p_prefabs)
	{
		const pgtest::Document document(p_document, "verdant-trial-arena.json");
		pg::JsonReader reader = document.reader();
		pg::HandcraftedBattleBoardDefinition definition =
			pg::parseHandcraftedBattleBoardDefinition(reader, p_prefabs);
		definition.id = "verdant-trial-arena";
		return definition;
	}
}

TEST(HandcraftedBattleBoardTest, ParsesTheExactSixKeySchema)
{
	const pg::Registry<pg::PrefabDefinition> registry = prefabs({{0, 0, 0}, {12, 2, 12}});
	const pg::HandcraftedBattleBoardDefinition parsed = parse(board(), registry);

	EXPECT_EQ(parsed.size, spk::Vector3Int(13, 3, 13));
	EXPECT_EQ(parsed.geometryPrefabId, "arena-geometry") << "the id, never a pointer into the prefab registry";
	EXPECT_EQ(parsed.playerApproach, spk::VoxelOrientation::PositiveZ);
	EXPECT_EQ(parsed.deploymentDepth, 2);
	EXPECT_EQ(pg::approachAxisExtent(parsed), 13);
	EXPECT_EQ(pg::deploymentRowWidth(parsed), 13);

	// The player walks in along the approach, so it lands on the low-z edge and the enemy waits on
	// the far one.
	const pg::BoardDeploymentStrip enemy = pg::enemyDeploymentStrip(parsed);
	EXPECT_EQ(enemy.minZ, 11);
	EXPECT_EQ(enemy.maxZ, 12);
	EXPECT_TRUE(enemy.contains(0, 12));
	EXPECT_FALSE(enemy.contains(0, 10));
	const pg::BoardDeploymentStrip player = pg::playerDeploymentStrip(parsed);
	EXPECT_EQ(player.minZ, 0);
	EXPECT_EQ(player.maxZ, 1);

	// Missing keys and unknown keys both fail; all six are required.
	EXPECT_THROW(
		auto value = parse(
			R"({"version": 1, "displayNameKey": "battle-board.fixture.name", "size": [13, 3, 13],
			   "geometryPrefab": "arena-geometry", "playerApproach": "positiveZ"})",
			registry),
		pg::JsonError);
	EXPECT_THROW(
		auto value = parse(
			R"({"version": 1, "displayNameKey": "battle-board.fixture.name", "size": [13, 3, 13],
			   "geometryPrefab": "arena-geometry", "playerApproach": "positiveZ", "deploymentDepth": 2,
			   "opponentPlacement": {"type": "seededRandom"}})",
			registry),
		pg::JsonError)
		<< "placement belongs to the encounter, not to the arena";
	EXPECT_THROW(
		auto value = parse(
			R"({"version": 2, "displayNameKey": "battle-board.fixture.name", "size": [13, 3, 13],
			   "geometryPrefab": "arena-geometry", "playerApproach": "positiveZ", "deploymentDepth": 2})",
			registry),
		pg::JsonError);
}

TEST(HandcraftedBattleBoardTest, EnforcesEverySizeBoundAndParity)
{
	const pg::Registry<pg::PrefabDefinition> registry = prefabs({{0, 0, 0}});

	EXPECT_NO_THROW(auto value = parse(board("[5, 3, 5]", "positiveZ", "1"), registry));
	EXPECT_NO_THROW(auto value = parse(board("[31, 64, 31]", "positiveZ", "2"), registry));

	// Odd X and Z, so the board has a single centre column and row.
	EXPECT_THROW(auto value = parse(board("[12, 3, 13]"), registry), pg::JsonError);
	EXPECT_THROW(auto value = parse(board("[13, 3, 12]"), registry), pg::JsonError);
	// Sides in [5, 31], height in [3, 64].
	EXPECT_THROW(auto value = parse(board("[3, 3, 13]"), registry), pg::JsonError);
	EXPECT_THROW(auto value = parse(board("[33, 3, 13]"), registry), pg::JsonError);
	EXPECT_THROW(auto value = parse(board("[13, 2, 13]"), registry), pg::JsonError);
	EXPECT_THROW(auto value = parse(board("[13, 65, 13]"), registry), pg::JsonError);
	// Wrong shape entirely.
	EXPECT_THROW(auto value = parse(board("[13, 13]"), registry), pg::JsonError);
	EXPECT_THROW(auto value = parse(board(R"("13x3x13")"), registry), pg::JsonError);
}

TEST(HandcraftedBattleBoardTest, EnforcesDeploymentDepthOnBothApproachAxes)
{
	const pg::Registry<pg::PrefabDefinition> registry = prefabs({{0, 0, 0}});

	// Both strips have to fit in the approach axis: 2 * depth <= extent.
	EXPECT_NO_THROW(auto value = parse(board("[5, 3, 13]", "positiveZ", "6"), registry));
	EXPECT_THROW(auto value = parse(board("[5, 3, 13]", "positiveZ", "7"), registry), pg::JsonError);
	// The same board approached along X is far tighter, because X is the short axis there.
	EXPECT_NO_THROW(auto value = parse(board("[5, 3, 13]", "negativeX", "2"), registry));
	EXPECT_THROW(auto value = parse(board("[5, 3, 13]", "negativeX", "3"), registry), pg::JsonError);

	EXPECT_THROW(auto value = parse(board("[13, 3, 13]", "positiveZ", "0"), registry), pg::JsonError);
	EXPECT_THROW(auto value = parse(board("[13, 3, 13]", "positiveZ", "-1"), registry), pg::JsonError);

	// Every horizontal approach literal parses; a vertical one has no deployment strip and does not
	// exist in the vocabulary.
	for (const std::string_view approach : {"positiveX", "negativeX", "positiveZ", "negativeZ"})
	{
		EXPECT_NO_THROW(auto value = parse(board("[13, 3, 13]", approach, "2"), registry)) << approach;
	}
	EXPECT_THROW(auto value = parse(board("[13, 3, 13]", "positiveY", "2"), registry), pg::JsonError);
}

TEST(HandcraftedBattleBoardTest, ValidatesThePrefabAtTheIdentityTransformAndRejectsRatherThanClips)
{
	// Applying the prefab with destination == pivot means an applied voxel lands on its authored
	// position, which is exactly what the bounds check reads - and what step 05 will stamp.
	EXPECT_NO_THROW(auto value = parse(board(), prefabs({{0, 0, 0}, {6, 1, 6}, {12, 2, 12}})));

	// One cell outside the box rejects the board. Prefab::applyTo would have clipped it away, and
	// an arena silently missing a wall is worse than one that refuses to load.
	EXPECT_THROW(auto value = parse(board(), prefabs({{0, 0, 0}, {13, 0, 0}})), pg::JsonError);
	EXPECT_THROW(auto value = parse(board(), prefabs({{0, 0, 0}, {0, 3, 0}})), pg::JsonError);
	EXPECT_THROW(auto value = parse(board(), prefabs({{0, 0, 0}, {0, 0, 13}})), pg::JsonError);
	EXPECT_THROW(auto value = parse(board(), prefabs({{-1, 0, 0}})), pg::JsonError);
	EXPECT_THROW(auto value = parse(board(), prefabs({{0, -1, 0}})), pg::JsonError);

	// A prefab pivot does not move the authored positions, because the destination is the pivot.
	EXPECT_NO_THROW(auto value = parse(board(), prefabs({{0, 0, 0}, {12, 2, 12}}, {6, 0, 6})));
	EXPECT_THROW(auto value = parse(board(), prefabs({{0, 0, 0}, {13, 2, 12}}, {6, 0, 6})), pg::JsonError);

	// And an unknown prefab is a missing reference, not an empty arena.
	EXPECT_THROW(auto value = parse(board("[13, 3, 13]", "positiveZ", "2", "ghost-geometry"), prefabs({{0, 0, 0}})), pg::JsonError);
}
