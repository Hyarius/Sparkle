#include <gtest/gtest.h>

#include "spk_update_module.hpp"
#include "window_test_utils.hpp"

TEST(UpdateModuleTest, UpdateIsSafeWhenUnbound)
{
	spk::UpdateModule module;

	EXPECT_NO_THROW(module.update());
}

TEST(UpdateModuleTest, UpdateBuildsAndForwardsTickToBoundWidget)
{
	spk::UpdateModule module;
	sparkle_test::RecordingWidget widget("UpdateWidget");

	widget.activate();
	module.bind(&widget);

	module.update();

	EXPECT_EQ(widget.updateCount, 1);
}

TEST(UpdateModuleTest, BoundInputsAreInjectedIntoConstructedTick)
{
	spk::UpdateModule module;
	sparkle_test::RecordingWidget widget("UpdateWidget");
	spk::Mouse mouse;
	spk::Keyboard keyboard;

	widget.activate();
	module.bind(&widget);
	module.bindInputs(&mouse, &keyboard);

	module.update();

	EXPECT_EQ(widget.updateCount, 1);
	EXPECT_EQ(widget.lastTickMouse, &mouse);
	EXPECT_EQ(widget.lastTickKeyboard, &keyboard);
}
