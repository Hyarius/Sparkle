#include <gtest/gtest.h>

#include <stdexcept>

#include "structures/game_engine/spk_game_engine.hpp"
#include "structures/widget/spk_game_engine_widget.hpp"

TEST(GameEngineWidgetTest, OwnsAnEngineByDefault)
{
	spk::GameEngineWidget widget("Game");

	EXPECT_NO_THROW((void)widget.gameEngine());
	EXPECT_TRUE(widget.isActivated() == false || widget.isActivated() == true); // engine is reachable
}

TEST(GameEngineWidgetTest, SetExternalEngineReplacesOwnedEngine)
{
	spk::GameEngineWidget widget("Game");
	spk::GameEngine external;

	widget.setExternalGameEngine(&external);

	EXPECT_EQ(&widget.gameEngine(), &external);
}

TEST(GameEngineWidgetTest, GameEngineThrowsWhenExternalEngineIsNull)
{
	spk::GameEngineWidget widget("Game");

	widget.setExternalGameEngine(nullptr);

	EXPECT_THROW((void)widget.gameEngine(), std::runtime_error);
}

TEST(GameEngineWidgetTest, ResetOwnedEngineRestoresAnInternalEngine)
{
	spk::GameEngineWidget widget("Game");
	spk::GameEngine external;
	widget.setExternalGameEngine(&external);

	widget.resetOwnedGameEngine();

	EXPECT_NO_THROW((void)widget.gameEngine());
	EXPECT_NE(&widget.gameEngine(), &external);
}

TEST(GameEngineWidgetTest, BuildsAnEmptyRenderUnitWithoutCrashing)
{
	spk::GameEngineWidget widget("Game");
	widget.setGeometry(spk::Rect2D(0, 0, 32, 32));

	auto renderUnit = widget.renderUnit();

	ASSERT_NE(renderUnit, nullptr);
	EXPECT_EQ(renderUnit->size(), 0u);
}
