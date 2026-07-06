#include "core/exploration_mode.hpp"
#include "core/game_context.hpp"
#include "core/registries.hpp"
#include "creatures/creature_unit.hpp"
#include "ui/exploration_hud.hpp"

#include <gtest/gtest.h>

#include <filesystem>

namespace
{
	[[nodiscard]] const pg::Registries &registries()
	{
		static const pg::Registries result = [] {
			pg::Registries loaded;
			loaded.loadAll(std::filesystem::path(PG_RESOURCE_DIR) / "data");
			return loaded;
		}();
		return result;
	}
}

TEST(ExplorationHud, TracksModeZonePartyHealthAndInteractionPrompt)
{
	pg::GameContext game;
	game.newGame(registries());
	game.world.activeMap = &registries().maps().get("m1-testground");
	game.world.activeBiome = &registries().biomes().get("forest");
	pg::ExplorationHud hud("ExplorationHud", nullptr);
	hud.bind(game);
	EXPECT_FALSE(hud.isActivated());
	EXPECT_EQ(hud.partyCount(), 2u);
	EXPECT_NE(hud.zoneText().find("m1-testground"), std::string::npos);

	pg::ExplorationMode mode(game);
	mode.enter();
	EXPECT_TRUE(hud.isActivated());
	game.world.activeMap = &registries().maps().get("test-tunnel");
	game.world.activeBiome = &registries().biomes().get("cave");
	game.events.worldChanged.trigger();
	EXPECT_NE(hud.zoneText().find("Cave Tunnel"), std::string::npos);
	game.world.activeMap = nullptr;
	game.world.activeBiome = &registries().biomes().get("meadow");
	game.events.worldChanged.trigger();
	EXPECT_EQ(hud.zoneText(), "Generated World - Meadow");

	game.player.team[0]->currentHealth = 2;
	game.events.partyChanged.trigger();
	EXPECT_EQ(hud.partyCard(0).hpBar().current(), 2.0f);
	game.events.interactionPromptChanged.trigger("Enter");
	EXPECT_EQ(hud.promptText(), "Enter");
	mode.exit();
	EXPECT_FALSE(hud.isActivated());
}

TEST(ExplorationHud, RebindCycleDoesNotAccumulateEventContracts)
{
	pg::GameContext game;
	game.newGame(registries());
	pg::ExplorationHud hud("ExplorationHud", nullptr);
	for (int cycle = 0; cycle < 3; ++cycle)
	{
		hud.bind(game);
		EXPECT_EQ(game.events.worldChanged.nbContracts(), 1u);
		EXPECT_EQ(game.events.partyChanged.nbContracts(), 1u);
		hud.unbind();
		EXPECT_EQ(game.events.worldChanged.nbContracts(), 0u);
		EXPECT_EQ(game.events.partyChanged.nbContracts(), 0u);
	}
}
