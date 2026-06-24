#include <gtest/gtest.h>

#include <stdexcept>

#include "structures/widget/spk_checkable_icon_button.hpp"
#include "structures/widget/spk_icon_button.hpp"
#include "structures/widget/spk_widget_visual_test_helpers.hpp"
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

TEST(IconButtonTest, MinimalSizeAccountsForIconAndFrame)
{
	spk::IconButton button("Icon");

	const spk::Vector2UInt minimalSize = button.minimalSize();

	EXPECT_GT(minimalSize.x, button.releasedBackground().cornerSize().x * 2u);
	EXPECT_GT(minimalSize.y, button.releasedBackground().cornerSize().y * 2u);
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
	EXPECT_TRUE(button.uncheckedButton().isActivated());
	EXPECT_FALSE(button.checkedButton().isActivated());
	EXPECT_FALSE(button.uncheckedButton().hasIcon());
	EXPECT_TRUE(button.checkedButton().hasIcon());
	EXPECT_NE(button.checkedButton().iconset(), nullptr);
	EXPECT_EQ(button.checkedIconSpriteID(), 8u);
}

TEST(CheckableIconButtonTest, DefaultUncheckedStateStaysIconlessAfterStyleRefresh)
{
	spk::CheckableIconButton button("Checkable");

	button.applyStyle(spk::WidgetStyle::Collection::style(spk::WidgetStyle::Collection::Default));

	EXPECT_FALSE(button.uncheckedButton().hasIcon());
	EXPECT_TRUE(button.checkedButton().hasIcon());
	EXPECT_EQ(button.checkedIconSpriteID(), 8u);
}

TEST(CheckableIconButtonTest, ConstructionAssignsOneIconPerState)
{
	spk::CheckableIconButton button("Checkable", 8, 9);

	EXPECT_EQ(button.uncheckedIconSpriteID(), 8u);
	EXPECT_EQ(button.checkedIconSpriteID(), 9u);
	EXPECT_EQ(button.uncheckedButton().iconSpriteID(), 8u);
	EXPECT_EQ(button.checkedButton().iconSpriteID(), 9u);
}

TEST(CheckableIconButtonTest, SetCheckedSwapsActiveButton)
{
	spk::CheckableIconButton button("Checkable", 8, 9);

	button.setChecked(true);

	EXPECT_TRUE(button.isChecked());
	EXPECT_TRUE(button.checkedButton().isActivated());
	EXPECT_FALSE(button.uncheckedButton().isActivated());

	button.setChecked(false);

	EXPECT_FALSE(button.isChecked());
	EXPECT_TRUE(button.uncheckedButton().isActivated());
	EXPECT_FALSE(button.checkedButton().isActivated());
}

TEST(CheckableIconButtonTest, SetIconSpriteIDUpdatesStateButton)
{
	spk::CheckableIconButton button("Checkable", 8, 9);

	button.setUncheckedIconSpriteID(2);
	button.setCheckedIconSpriteID(3);

	EXPECT_EQ(button.uncheckedButton().iconSpriteID(), 2u);
	EXPECT_EQ(button.checkedButton().iconSpriteID(), 3u);
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

TEST(IconButtonVisualTest, RendersDefaultIcon)
{
	const spk::Rect2D captureRect(0, 0, 48, 48);

	spk::IconButton button("Icon");

	const sparkle_test::ImageComparisonResult result =
		spk::test::compareSnapshot(button, "IconButtonVisual", "default", captureRect);

	EXPECT_TRUE(result.matches);
}

TEST(IconButtonVisualTest, RendersDifferentIconID)
{
	const spk::Rect2D captureRect(0, 0, 48, 48);

	spk::IconButton button("Icon");
	button.setIconSpriteID(3);

	const sparkle_test::ImageComparisonResult result =
		spk::test::compareSnapshot(button, "IconButtonVisual", "icon_id_3", captureRect);

	EXPECT_TRUE(result.matches);
}

TEST(IconButtonVisualTest, RendersFlat)
{
	const spk::Rect2D captureRect(0, 0, 48, 48);

	spk::IconButton button("Icon");
	button.setFlat(true);

	const sparkle_test::ImageComparisonResult result =
		spk::test::compareSnapshot(button, "IconButtonVisual", "flat", captureRect);

	EXPECT_TRUE(result.matches);
}

TEST(CheckableIconButtonVisualTest, RendersUnchecked)
{
	const spk::Rect2D captureRect(0, 0, 48, 48);

	spk::CheckableIconButton button("Checkable", 0, 1);

	const sparkle_test::ImageComparisonResult result =
		spk::test::compareSnapshot(button, "CheckableIconButtonVisual", "unchecked", captureRect);

	EXPECT_TRUE(result.matches);
}

TEST(CheckableIconButtonVisualTest, RendersChecked)
{
	const spk::Rect2D captureRect(0, 0, 48, 48);

	spk::CheckableIconButton button("Checkable", 0, 1);
	button.setChecked(true);

	const sparkle_test::ImageComparisonResult result =
		spk::test::compareSnapshot(button, "CheckableIconButtonVisual", "checked", captureRect);

	EXPECT_TRUE(result.matches);
}
