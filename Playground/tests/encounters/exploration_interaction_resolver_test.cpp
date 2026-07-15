#include <gtest/gtest.h>

#include "core/deterministic_random.hpp"
#include "core/paths.hpp"
#include "core/registries.hpp"
#include "encounters/exploration_interaction_resolver.hpp"
#include "player/new_game_definition.hpp"
#include "world/generator/world_plan.hpp"
#include "world/voxel_world.hpp"

#include "structures/voxel/spk_voxel_chunk.hpp"

#include <memory>
#include <unordered_map>
#include <variant>

namespace
{
	const pg::Registries &loadedRegistries()
	{
		static pg::Registries registries;
		static const bool loaded = [] {
			registries.loadAll(pg::resourceRoot() / "data");
			return true;
		}();
		(void)loaded;
		return registries;
	}

	std::unique_ptr<pg::VoxelWorld> emptyWorld(const pg::Registries &p_registries)
	{
		return std::make_unique<pg::VoxelWorld>(p_registries.voxels(), [](spk::VoxelChunk &) {
		});
	}
}

TEST(ExplorationInteractionResolverTest, PortalWinsWithoutInspectingBushOrConsumingAnOrdinal)
{
	const pg::Registries &registries = loadedRegistries();
	pg::PlayerData player = pg::makeNewPlayerData(registries.newGame(), registries);
	const std::unique_ptr<pg::VoxelWorld> world = emptyWorld(registries);
	pg::WorldPlan plan;
	const spk::Vector3Int arrival{4, 12, -7};
	std::unordered_map<spk::Vector3Int, spk::Vector3Int> portals{{arrival, {19, 3, 22}}};
	pg::ExplorationInteractionResolver resolver(registries, *world, plan, portals, player, 455);

	const pg::ExplorationInteractionResolution result =
		resolver.resolvePlayerArrival(arrival, pg::VoxelOrientation::NegativeX, player.encounterSerial);
	ASSERT_TRUE(std::holds_alternative<pg::TeleportRequest>(result.interaction));
	EXPECT_EQ(std::get<pg::TeleportRequest>(result.interaction).destination, (spk::Vector3Int{19, 3, 22}));
	EXPECT_FALSE(result.consumesEncounterOrdinal);
}

TEST(ExplorationInteractionResolverTest, NamedDebugEncounterCarriesStableSeedsAndConsumesExactlyOneOrdinal)
{
	const pg::Registries &registries = loadedRegistries();
	pg::PlayerData player = pg::makeNewPlayerData(registries.newGame(), registries);
	const std::unique_ptr<pg::VoxelWorld> world = emptyWorld(registries);
	pg::WorldPlan plan;
	std::unordered_map<spk::Vector3Int, spk::Vector3Int> portals;
	pg::ExplorationInteractionResolver resolver(registries, *world, plan, portals, player, 0xabc123);

	const pg::ExplorationInteractionResolution result = resolver.resolveNamedEncounter(
		"debug-battle",
		{2, 8, 9},
		pg::VoxelOrientation::PositiveZ,
		17);
	ASSERT_TRUE(result.consumesEncounterOrdinal);
	ASSERT_TRUE(std::holds_alternative<pg::BattleStartRequest>(result.interaction));
	const pg::BattleStartRequest &request = std::get<pg::BattleStartRequest>(result.interaction);
	EXPECT_EQ(request.encounter.encounterDefinitionId, "debug-battle");
	EXPECT_EQ(request.stableEncounterInstanceId, "battle/debug/debug-battle/17");
	EXPECT_EQ(
		request.encounterResolutionSeed,
		pg::deterministic::deriveSeed(0xabc123, "battle/debug/debug-battle/17/encounter-resolution"));
	EXPECT_EQ(request.combatSeed, pg::deterministic::deriveSeed(0xabc123, "battle/debug/debug-battle/17/combat"));
	EXPECT_EQ(request.worldCell, (spk::Vector3Int{2, 8, 9}));
}

TEST(ExplorationInteractionResolverTest, InvalidAutoplayProfileDoesNotConsumeTheOrdinal)
{
	const pg::Registries &registries = loadedRegistries();
	pg::PlayerData player = pg::makeNewPlayerData(registries.newGame(), registries);
	const std::unique_ptr<pg::VoxelWorld> world = emptyWorld(registries);
	pg::WorldPlan plan;
	std::unordered_map<spk::Vector3Int, spk::Vector3Int> portals;
	pg::ExplorationInteractionResolver resolver(registries, *world, plan, portals, player, 12);

	const pg::ExplorationInteractionResolution result = resolver.resolveNamedEncounter(
		"debug-battle",
		{},
		pg::VoxelOrientation::PositiveZ,
		0,
		"missing-profile");
	EXPECT_FALSE(result.consumesEncounterOrdinal);
	EXPECT_TRUE(std::holds_alternative<std::monostate>(result.interaction));
}
