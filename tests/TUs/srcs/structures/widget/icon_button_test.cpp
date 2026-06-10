#include <gtest/gtest.h>

#include <stdexcept>

#include "structures/widget/spk_checkable_icon_button.hpp"
#include "structures/widget/spk_icon_button.hpp"
#include "structures/application/module/spk_mouse_module.hpp"
#include "structures/system/device/window/window_test_utils.hpp"

namespace
{
	void clickAt(spk::Widget& p_widget, const spk::Vector2Int& p_position)
	{
		spk::MouseModule mouseModule;
		mouseModule.bind(&p_widget);
		mouseModule.pushEvent(spk::MouseEventRecord(spk::makeEventRecord(spk::MouseMovedRecord{.position = p_position})));
		mouseModule.pushEvent(spk::MouseEventRecord(spk::makeEventRecord(spk::MouseButtonPressedRecord{.button = spk::Mouse::Left})));
		mouseModule.pushEvent(spk::MouseEventRecord(spk::makeEventRecord(spk::MouseButtonReleasedRecord{.button = spk::Mouse::Left})));
		mouseModule.processEvents();
	}
}

TEST(IconButtonTest, ConstructionUsesDefaultIconset)
{
	spk::IconButton button("Icon");

	EXPECT_TRUE(button.isActivated());
	EXPECT_TRUE(button.hasIcon());
	EXPECT_NE(button.iconset(), nullptr);
	EXPECT_EQ(button.iconSpriteID(), 0u);
}

TEST(IconButtonTest, SetIconSpriteIDUpdatesIconSection)
{
	spk::IconButton button("Icon");

	const auto firstSection = button.releasedIcon().section();
	button.setIconSpriteID(3);

	EXPECT_EQ(button.iconSpriteID(), 3u);
	EXPECT_NE(button.releasedIcon().section(), firstSection);
}

TEST(IconButtonTest, SetIconSpriteIDFromCoordinates)
{
	spk::IconButton button("Icon");

	button.setIconSpriteID(spk::Vector2UInt{2, 1});

	EXPECT_EQ(button.iconSpriteID(), 12u);
}

TEST(IconButtonTest, SetIconsetRejectsNull)
{
	spk::IconButton button("Icon");

	EXPECT_THROW(button.setIconset(nullptr), std::invalid_argument);
}

TEST(CheckableIconButtonTest, DefaultStateIsUnchecked)
{
	spk::CheckableIconButton button("Checkable");

	EXPECT_FALSE(button.isChecked());
	EXPECT_EQ(button.iconSpriteID(), button.uncheckedIconSpriteID());
}

TEST(CheckableIconButtonTest, SetCheckedSwapsIcon)
{
	spk::CheckableIconButton button("Checkable", 8, 9);

	button.setChecked(true);

	EXPECT_TRUE(button.isChecked());
	EXPECT_EQ(button.iconSpriteID(), 9u);

	button.setChecked(false);

	EXPECT_FALSE(button.isChecked());
	EXPECT_EQ(button.iconSpriteID(), 8u);
}

TEST(CheckableIconButtonTest, ToggleFlipsState)
{
	spk::CheckableIconButton button("Checkable");

	button.toggle();
	EXPECT_TRUE(button.isChecked());

	button.toggle();
	EXPECT_FALSE(button.isChecked());
}

TEST(CheckableIconButtonTest, ClickTogglesState)
{
	spk::CheckableIconButton button("Checkable");
	button.setGeometry(spk::Rect2D(0, 0, 40, 40));

	clickAt(button, {20, 20});
	EXPECT_TRUE(button.isChecked());

	clickAt(button, {20, 20});
	EXPECT_FALSE(button.isChecked());
}

TEST(CheckableIconButtonTest, StateCallbacksFireOnMatchingState)
{
	spk::CheckableIconButton button("Checkable");

	int checkedCount = 0;
	int uncheckedCount = 0;
	auto checkedContract = button.addStateCallback(true, [&checkedCount]() { ++checkedCount; });
	auto uncheckedContract = button.addStateCallback(false, [&uncheckedCount]() { ++uncheckedCount; });

	button.setChecked(true);
	EXPECT_EQ(checkedCount, 1);
	EXPECT_EQ(uncheckedCount, 0);

	button.setChecked(false);
	EXPECT_EQ(checkedCount, 1);
	EXPECT_EQ(uncheckedCount, 1);

	button.setChecked(false);
	EXPECT_EQ(uncheckedCount, 1);
}

TEST(CheckableIconButtonTest, SubscribeToStateReceivesNewValue)
{
	spk::CheckableIconButton button("Checkable");

	bool lastState = false;
	auto contract = button.subscribeToState([&lastState](const bool& p_state) { lastState = p_state; });

	button.setChecked(true);

	EXPECT_TRUE(lastState);
}
