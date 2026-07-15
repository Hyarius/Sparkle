#include <gtest/gtest.h>

#include "fixtures/board_fixture.hpp"
#include "support/json_fixture.hpp"

#include "board/board_builder.hpp"
#include "board/board_line_of_sight.hpp"
#include "board/handcrafted_battle_board_definition.hpp"
#include "voxel/shape_catalog.hpp"
#include "voxel/voxel_family_definition.hpp"
#include "voxel/voxel_registry.hpp"
#include "world/prefab_definition.hpp"

#include "structures/voxel/spk_prefab.hpp"

#include <memory>
#include <vector>

namespace
{
	// A tiny registry over the fixture's real voxels, so an arena is materialised from the same
	// parsers, shapes, costs and tags production content obeys. It outlives every board built from it.
	struct Arena
	{
		pg::ShapeCatalog shapes;
		pg::VoxelFamilyCatalog families;
		pg::VoxelRegistry voxels;

		Arena()
		{
			pgtest::loadFixtureVoxels(shapes, families, voxels);
		}
	};

	// A 5x5 stone floor, authored voxel by voxel so a case can place one cell exactly where it wants.
	[[nodiscard]] pg::PrefabDefinition floorPrefab(
		const pg::VoxelRegistry &p_voxels,
		const spk::Vector3Int &p_pivot = {0, 0, 0})
	{
		pg::PrefabDefinition definition;
		definition.id = "arena-geometry";
		const spk::VoxelRuntimeId ground = p_voxels.runtimeId("test-ground");
		for (int z = 0; z < 5; ++z)
		{
			for (int x = 0; x < 5; ++x)
			{
				definition.prefab.addVoxel({x, 0, z}, spk::VoxelCell{.id = ground});
			}
		}
		definition.prefab.setPivot(p_pivot);
		return definition;
	}

	[[nodiscard]] pg::HandcraftedBattleBoardDefinition boardDefinition(
		const spk::Vector3Int &p_size = {5, 3, 5},
		pg::VoxelOrientation p_approach = pg::VoxelOrientation::PositiveZ,
		int p_deploymentDepth = 1,
		const std::string &p_prefabId = "arena-geometry")
	{
		pg::HandcraftedBattleBoardDefinition definition;
		definition.id = "verdant-trial-arena";
		definition.displayNameKey = "battle-board.fixture.name";
		definition.size = p_size;
		definition.geometryPrefabId = p_prefabId;
		definition.playerApproach = p_approach;
		definition.deploymentDepth = p_deploymentDepth;
		return definition;
	}
}

TEST(HandcraftedBoardBuilderTest, MaterialisesThePrefabAtIdentityIntoAnOwnedImmutableGrid)
{
	Arena arena;
	const pg::PrefabDefinition prefab = floorPrefab(arena.voxels);
	const pg::HandcraftedBattleBoardDefinition definition = boardDefinition();

	const pg::BoardBuildResult built =
		pg::BoardBuilder::buildHandcrafted(definition, prefab, arena.voxels, 1, 1, 0.5);
	ASSERT_TRUE(built.board.has_value()) << built.error->diagnosticDetail;
	const pg::BoardData &board = *built.board;

	EXPECT_EQ(board.sourceDescriptor().kind, pg::BoardSourceKind::Handcrafted);
	EXPECT_EQ(board.sourceDescriptor().definitionId, std::optional<std::string>("verdant-trial-arena"));
	EXPECT_EQ(board.presentationOrigin(), spk::Vector3Int(0, 0, 0));
	EXPECT_FALSE(board.liveWorldAnchor().has_value());
	EXPECT_EQ(board.extent().size, spk::Vector2Int(5, 5));
	EXPECT_EQ(board.extent().traversalBounds.maximum, spk::Vector3Int(5, 3, 5)) << "its own box, not exploration Y bounds";

	// The authored voxel at (x, 0, z) is a board-local voxel at (x, 0, z): no rotation, no flip, no
	// translation. The floor is exactly the floor that was drawn.
	const spk::VoxelRuntimeId ground = arena.voxels.runtimeId("test-ground");
	for (int z = 0; z < 5; ++z)
	{
		for (int x = 0; x < 5; ++x)
		{
			EXPECT_EQ(board.cells().cell({x, 0, z}).id, ground) << x << "," << z;
			EXPECT_TRUE(board.isStandable({x, 0, z}));
		}
	}
	EXPECT_EQ(board.navigation().size(), 25U);
}

TEST(HandcraftedBoardBuilderTest, ANonZeroPivotAndExplicitCarveCellsDoNotTranslateTheGrid)
{
	Arena arena;
	// The floor, a wall pillar floating two cells up, and the cell under it explicitly carved empty.
	// The pivot is off the origin, and the destination is the pivot, so none of it moves.
	pg::PrefabDefinition prefab = floorPrefab(arena.voxels, {2, 0, 2});
	const spk::VoxelRuntimeId wall = arena.voxels.runtimeId("test-wall");
	prefab.prefab.addVoxel({2, 1, 2}, spk::VoxelCell{}); // an explicit carve entry, still bounds-checked
	prefab.prefab.addVoxel({2, 2, 2}, spk::VoxelCell{.id = wall});

	const pg::BoardBuildResult built =
		pg::BoardBuilder::buildHandcrafted(boardDefinition(), prefab, arena.voxels, 1, 1, 0.5);
	ASSERT_TRUE(built.board.has_value()) << built.error->diagnosticDetail;
	const pg::BoardData &board = *built.board;

	EXPECT_EQ(board.cells().cell({2, 2, 2}).id, wall) << "the pillar lands on its authored position";
	EXPECT_TRUE(board.cells().cell({2, 1, 2}).isEmpty()) << "the carved cell stays air";
	EXPECT_EQ(board.cells().cell({2, 0, 2}).id, arena.voxels.runtimeId("test-ground"));
}

TEST(HandcraftedBoardBuilderTest, AVoxelOutsideTheDeclaredBoxRejectsRatherThanClips)
{
	Arena arena;
	const pg::HandcraftedBattleBoardDefinition definition = boardDefinition();

	// buildHandcrafted rechecks the geometry itself rather than trusting the registry: a prefab voxel
	// at or past any upper bound, or below zero, fails before a single cell is written - never a grid
	// silently missing a wall.
	for (const spk::Vector3Int &outside :
		 {spk::Vector3Int{5, 0, 0}, spk::Vector3Int{0, 3, 0}, spk::Vector3Int{0, 0, 5}, spk::Vector3Int{-1, 0, 0}})
	{
		pg::PrefabDefinition prefab = floorPrefab(arena.voxels);
		prefab.prefab.addVoxel(outside, spk::VoxelCell{.id = arena.voxels.runtimeId("test-wall")});

		const pg::BoardBuildResult built =
			pg::BoardBuilder::buildHandcrafted(definition, prefab, arena.voxels, 1, 1, 0.5);
		ASSERT_FALSE(built.board.has_value()) << outside.toString();
		EXPECT_EQ(built.error->code, pg::BoardBuildErrorCode::PrefabVoxelOutOfBounds);
		EXPECT_EQ(built.error->cell, std::optional<spk::Vector3Int>(outside));
		EXPECT_EQ(built.error->prefabId, std::optional<std::string>("arena-geometry"));
	}
}

TEST(HandcraftedBoardBuilderTest, DefinitionAndPrefabMustAgreeOnIdentity)
{
	Arena arena;
	const pg::PrefabDefinition prefab = floorPrefab(arena.voxels);

	// The board names one prefab and is handed another: a mismatch is a broken content link, named
	// with both ids.
	const pg::HandcraftedBattleBoardDefinition mismatched = boardDefinition({5, 3, 5}, pg::VoxelOrientation::PositiveZ, 1, "some-other-prefab");
	const pg::BoardBuildResult built =
		pg::BoardBuilder::buildHandcrafted(mismatched, prefab, arena.voxels, 1, 1, 0.5);
	ASSERT_FALSE(built.board.has_value());
	EXPECT_EQ(built.error->code, pg::BoardBuildErrorCode::InvalidHandcraftedDefinition);
	EXPECT_EQ(built.error->prefabId, std::optional<std::string>("arena-geometry"));
}

TEST(HandcraftedBoardBuilderTest, AnUnplayableArenaIsATypedFailureNotALiveWorldFallback)
{
	Arena arena;
	const pg::PrefabDefinition prefab = floorPrefab(arena.voxels);

	// Five cells per deployment strip, six creatures asked for: the arena is refused. There is no
	// fallback to live terrain - a Gym is its authored board or nothing.
	const pg::BoardBuildResult built =
		pg::BoardBuilder::buildHandcrafted(boardDefinition(), prefab, arena.voxels, 6, 1, 0.5);
	ASSERT_FALSE(built.board.has_value());
	EXPECT_EQ(built.error->code, pg::BoardBuildErrorCode::InsufficientDeploymentCapacity);
	EXPECT_EQ(built.error->availablePlayerSeats, 5U);
	EXPECT_EQ(built.error->boardOrEncounterId, "verdant-trial-arena");
}

TEST(HandcraftedBoardBuilderTest, TheBoardRetainsItsGridAndIsAlwaysCurrentWithNoWorld)
{
	// The registry is a lifetime borrow (Registries guarantees it in production), so it stays; the
	// prefab and the build-result wrapper are construction temporaries and are destroyed. The board
	// still reads its cells, because the grid is owned by the source rather than borrowed from a
	// temporary.
	Arena arena;
	std::optional<pg::BoardData> board;
	{
		const pg::PrefabDefinition prefab = floorPrefab(arena.voxels, {2, 0, 2});
		pg::BoardBuildResult built =
			pg::BoardBuilder::buildHandcrafted(boardDefinition(), prefab, arena.voxels, 1, 1, 0.5);
		ASSERT_TRUE(built.board.has_value());
		board = std::move(built.board);
	}

	EXPECT_EQ(board->cells().cell({0, 0, 0}).id, arena.voxels.runtimeId("test-ground"))
		<< "the grid survived the prefab and the build result that made it";
	EXPECT_EQ(board->movementCost({4, 0, 4}), 1);
	// No world, chunk, pin, streamer, fluid, seed or RNG object exists in this test, and the board is
	// current anyway - because its terrain cannot move.
	EXPECT_TRUE(board->terrainIsCurrent());
	EXPECT_NO_THROW(board->requireCurrentTerrain());
	EXPECT_FALSE(board->liveTerrainStamp().has_value());
}

TEST(HandcraftedBoardBuilderTest, TwoBuildsAreTopologyAndDescriptorEqualAndConsumeNoRng)
{
	Arena firstArena;
	Arena secondArena;
	const pg::PrefabDefinition firstPrefab = floorPrefab(firstArena.voxels);
	const pg::PrefabDefinition secondPrefab = floorPrefab(secondArena.voxels);

	const pg::BoardBuildResult first =
		pg::BoardBuilder::buildHandcrafted(boardDefinition(), firstPrefab, firstArena.voxels, 1, 1, 0.5);
	const pg::BoardBuildResult second =
		pg::BoardBuilder::buildHandcrafted(boardDefinition(), secondPrefab, secondArena.voxels, 1, 1, 0.5);
	ASSERT_TRUE(first.board.has_value());
	ASSERT_TRUE(second.board.has_value());

	// Same descriptor, same border ring, same deployment - the materialisation depends on the
	// definition and the prefab, and on nothing else: no seed, no ordinal, no chunk, no RNG draw.
	EXPECT_EQ(first.board->sourceDescriptor(), second.board->sourceDescriptor());
	EXPECT_EQ(first.board->borderCells(), second.board->borderCells());
	EXPECT_EQ(
		std::vector<pg::BoardCell>(
			first.board->deployment().playerCells.begin(), first.board->deployment().playerCells.end()),
		std::vector<pg::BoardCell>(
			second.board->deployment().playerCells.begin(), second.board->deployment().playerCells.end()));
	EXPECT_TRUE(first.board->terrainIsCurrent());
	EXPECT_NO_THROW(first.board->requireCurrentTerrain());
}

TEST(HandcraftedBoardBuilderTest, MatchesAnEquivalentSyntheticFixture)
{
	Arena arena;
	const pg::PrefabDefinition prefab = floorPrefab(arena.voxels);
	const pg::BoardBuildResult built =
		pg::BoardBuilder::buildHandcrafted(boardDefinition(), prefab, arena.voxels, 1, 1, 0.5);
	ASSERT_TRUE(built.board.has_value());
	const pg::BoardData &arenaBoard = *built.board;

	// A synthetic ASCII fixture of the same shape: same nav topology, same borders, same deployment,
	// same movement costs, same LoS answer. Two roads to the same frozen board.
	pgtest::BoardFixture fixture(pgtest::BoardFixture::flat(5, 5));
	const pg::BoardData &fixtureBoard = fixture.board();

	EXPECT_EQ(arenaBoard.navigation().size(), fixtureBoard.navigation().size());
	EXPECT_EQ(arenaBoard.borderCells(), fixtureBoard.borderCells());
	EXPECT_EQ(
		std::vector<pg::BoardCell>(
			arenaBoard.deployment().playerCells.begin(), arenaBoard.deployment().playerCells.end()),
		std::vector<pg::BoardCell>(
			fixtureBoard.deployment().playerCells.begin(), fixtureBoard.deployment().playerCells.end()));
	EXPECT_EQ(arenaBoard.movementCost({2, 0, 2}), fixtureBoard.movementCost({2, 0, 2}));
	EXPECT_EQ(
		pg::BoardLineOfSight::trace(arenaBoard, {0, 0, 0}, {4, 0, 4}).clear,
		pg::BoardLineOfSight::trace(fixtureBoard, {0, 0, 0}, {4, 0, 4}).clear);
}

TEST(HandcraftedBoardBuilderTest, LoadsAStrictBoardAndPrefabThroughTheRealParsers)
{
	Arena arena;

	// The prefab and the board both go through their real parsers: the strict schema, the identity
	// bounds check, the content-id rules. A parse-transaction failure catalogue lives in
	// handcrafted_battle_board_test.cpp; this proves the parsed pair then materialises.
	const pgtest::Document prefabDocument(
		R"({"version": 1, "palette": {"g": "test-ground"},
			"fill": [{"min": [0, 0, 0], "max": [4, 0, 4], "voxel": "g"}]})",
		"arena-geometry.json");
	pg::JsonReader prefabReader = prefabDocument.reader();
	pg::PrefabDefinition prefab = pg::parsePrefabDefinition(prefabReader, arena.voxels);
	prefab.id = "arena-geometry";

	pg::Registry<pg::PrefabDefinition> prefabs;
	prefabs.add("arena-geometry", prefab);

	const pgtest::Document boardDocument(
		R"({"version": 1, "displayNameKey": "battle-board.fixture.name", "size": [5, 3, 5],
			"geometryPrefab": "arena-geometry", "playerApproach": "positiveZ", "deploymentDepth": 1})",
		"verdant-trial-arena.json");
	pg::JsonReader boardReader = boardDocument.reader();
	pg::HandcraftedBattleBoardDefinition definition =
		pg::parseHandcraftedBattleBoardDefinition(boardReader, prefabs);
	definition.id = "verdant-trial-arena";

	const pg::BoardBuildResult built =
		pg::BoardBuilder::buildHandcrafted(definition, prefab, arena.voxels, 1, 1, 0.5);
	ASSERT_TRUE(built.board.has_value()) << built.error->diagnosticDetail;
	EXPECT_EQ(built.board->navigation().size(), 25U);
	EXPECT_EQ(built.board->movementCost({0, 0, 0}), 1);
}
