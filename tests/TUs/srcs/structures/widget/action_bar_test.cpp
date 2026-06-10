#include <gtest/gtest.h>

#include <stdexcept>

#include "structures/widget/spk_action_bar.hpp"
#include "structures/application/module/spk_mouse_module.hpp"
#include "structures/system/device/window/window_test_utils.hpp"

TEST(MenuBarTest, AddMenuCreatesDeactivatedMenu)
{
	spk::MenuBar menuBar("MenuBar");
	menuBar.setGeometry(spk::Rect2D(0, 0, 400, 300));

	spk::MenuBar::Menu* menu = menuBar.addMenu("File");

	ASSERT_NE(menu, nullptr);
	EXPECT_EQ(menuBar.nbMenu(), 1u);
	EXPECT_FALSE(menu->isActivated());
}

TEST(MenuBarTest, AddItemRegistersItems)
{
	spk::MenuBar menuBar("MenuBar");
	menuBar.setGeometry(spk::Rect2D(0, 0, 400, 300));

	spk::MenuBar::Menu* menu = menuBar.addMenu("File");
	menu->addItem("Open", []() {});
	menu->addItem("Save", []() {});
	menu->addBreak();
	menu->addItem("Quit", []() {});

	EXPECT_EQ(menu->nbItem(), 4u);
	EXPECT_GT(menu->minimalSize().y, 0u);
}

TEST(MenuBarTest, ClickingMenuButtonTogglesMenu)
{
	spk::MenuBar menuBar("MenuBar");
	menuBar.setGeometry(spk::Rect2D(0, 0, 400, 300));

	spk::MenuBar::Menu* menu = menuBar.addMenu("File");
	menu->addItem("Open", []() {});

	const spk::Rect2D buttonRect = menuBar.menuButton(0)->viewport().geometry();
	const spk::Vector2Int clickPosition = {
		buttonRect.x() + static_cast<int>(buttonRect.width() / 2),
		buttonRect.y() + static_cast<int>(buttonRect.height() / 2)};

	spk::MouseModule mouseModule;
	mouseModule.bind(&menuBar);
	mouseModule.pushEvent(spk::MouseEventRecord(spk::makeEventRecord(spk::MouseMovedRecord{.position = clickPosition})));
	mouseModule.pushEvent(spk::MouseEventRecord(spk::makeEventRecord(spk::MouseButtonPressedRecord{.button = spk::Mouse::Left})));
	mouseModule.pushEvent(spk::MouseEventRecord(spk::makeEventRecord(spk::MouseButtonReleasedRecord{.button = spk::Mouse::Left})));
	mouseModule.processEvents();

	EXPECT_TRUE(menu->isActivated());
}

TEST(MenuBarTest, ItemClickTriggersCallbackAndClosesMenu)
{
	spk::MenuBar menuBar("MenuBar");
	menuBar.setGeometry(spk::Rect2D(0, 0, 400, 300));

	int callCount = 0;
	spk::MenuBar::Menu* menu = menuBar.addMenu("File");
	spk::MenuBar::Menu::Item* item = menu->addItem("Open", [&callCount]() { ++callCount; });
	menu->activate();
	menuBar.setGeometry(spk::Rect2D(0, 0, 400, 300));

	const spk::Rect2D itemRect = item->viewport().geometry();
	const spk::Vector2Int clickPosition = {
		itemRect.x() + 2,
		itemRect.y() + static_cast<int>(itemRect.height() / 2)};

	spk::MouseModule mouseModule;
	mouseModule.bind(&menuBar);
	mouseModule.pushEvent(spk::MouseEventRecord(spk::makeEventRecord(spk::MouseMovedRecord{.position = clickPosition})));
	mouseModule.pushEvent(spk::MouseEventRecord(spk::makeEventRecord(spk::MouseButtonPressedRecord{.button = spk::Mouse::Left})));
	mouseModule.pushEvent(spk::MouseEventRecord(spk::makeEventRecord(spk::MouseButtonReleasedRecord{.button = spk::Mouse::Left})));
	mouseModule.processEvents();

	EXPECT_EQ(callCount, 1);
	EXPECT_FALSE(menu->isActivated());
}

TEST(MenuBarTest, BreakRejectsInvalidSpriteSheet)
{
	spk::MenuBar menuBar("MenuBar");
	spk::MenuBar::Menu* menu = menuBar.addMenu("File");
	spk::MenuBar::Menu::Break* breakItem = menu->addBreak();

	EXPECT_THROW(breakItem->setSpriteSheet(nullptr), std::invalid_argument);
}

TEST(MenuBarTest, MenuButtonIndexOutOfRangeThrows)
{
	spk::MenuBar menuBar("MenuBar");

	EXPECT_THROW((void)menuBar.menuButton(0), std::out_of_range);
}

TEST(MenuBarTest, BreakSetSpriteSheetWrongSizeThrows)
{
	spk::MenuBar menuBar("MenuBar");
	spk::MenuBar::Menu* menu = menuBar.addMenu("File");
	spk::MenuBar::Menu::Break* breakItem = menu->addBreak();

	// The nine-slice spritesheet has 3x3 sprites (not 3x1), so it should be rejected.
	auto wrongSheet = spk::WidgetStyle::makeDefault().nineSliceSpriteSheet();

	EXPECT_THROW(breakItem->setSpriteSheet(wrongSheet), std::invalid_argument);
}

TEST(MenuBarTest, BreakSetHeightUpdatesGetter)
{
	spk::MenuBar menuBar("MenuBar");
	spk::MenuBar::Menu* menu = menuBar.addMenu("File");
	spk::MenuBar::Menu::Break* breakItem = menu->addBreak();

	breakItem->setHeight(6);

	EXPECT_EQ(breakItem->height(), 6u);
}

TEST(MenuBarTest, SetHeightUpdatesGetter)
{
	spk::MenuBar menuBar("MenuBar");
	menuBar.setGeometry(spk::Rect2D(0, 0, 400, 300));

	menuBar.setHeight(30);

	EXPECT_EQ(menuBar.height(), 30u);
}

TEST(MenuBarTest, ClickingMenuButtonWithNoItemsDoesNotOpenMenu)
{
	spk::MenuBar menuBar("MenuBar");
	menuBar.setGeometry(spk::Rect2D(0, 0, 400, 300));

	spk::MenuBar::Menu* menu = menuBar.addMenu("Empty");

	const spk::Rect2D buttonRect = menuBar.menuButton(0)->viewport().geometry();
	const spk::Vector2Int clickPos = {
		buttonRect.x() + static_cast<int>(buttonRect.width() / 2),
		buttonRect.y() + static_cast<int>(buttonRect.height() / 2)};

	spk::MouseModule mouseModule;
	mouseModule.bind(&menuBar);
	mouseModule.pushEvent(spk::MouseEventRecord(spk::makeEventRecord(spk::MouseMovedRecord{.position = clickPos})));
	mouseModule.pushEvent(spk::MouseEventRecord(spk::makeEventRecord(spk::MouseButtonPressedRecord{.button = spk::Mouse::Left})));
	mouseModule.pushEvent(spk::MouseEventRecord(spk::makeEventRecord(spk::MouseButtonReleasedRecord{.button = spk::Mouse::Left})));
	mouseModule.processEvents();

	EXPECT_FALSE(menu->isActivated());
}

TEST(MenuBarTest, ClickOutsideOpenMenuClosesIt)
{
	spk::MenuBar menuBar("MenuBar");
	menuBar.setGeometry(spk::Rect2D(0, 0, 400, 300));

	spk::MenuBar::Menu* menu = menuBar.addMenu("File");
	menu->addItem("Open", []() {});
	menu->activate();
	menuBar.setGeometry(spk::Rect2D(0, 0, 400, 300));

	ASSERT_TRUE(menu->isActivated());

	// Click well outside the menu (which is narrow and anchored near the left)
	const spk::Vector2Int outsidePos = {350, 200};

	spk::MouseModule mouseModule;
	mouseModule.bind(&menuBar);
	mouseModule.pushEvent(spk::MouseEventRecord(spk::makeEventRecord(spk::MouseMovedRecord{.position = outsidePos})));
	mouseModule.pushEvent(spk::MouseEventRecord(spk::makeEventRecord(spk::MouseButtonPressedRecord{.button = spk::Mouse::Left})));
	mouseModule.processEvents();

	EXPECT_FALSE(menu->isActivated());
}

TEST(MenuBarTest, AddItemWithNullCallbackDoesNotCrashOnClick)
{
	spk::MenuBar menuBar("MenuBar");
	menuBar.setGeometry(spk::Rect2D(0, 0, 400, 300));

	spk::MenuBar::Menu* menu = menuBar.addMenu("File");
	spk::MenuBar::Menu::Item* item = menu->addItem("NoOp", nullptr);
	menu->activate();
	menuBar.setGeometry(spk::Rect2D(0, 0, 400, 300));

	const spk::Rect2D itemRect = item->viewport().geometry();
	const spk::Vector2Int clickPos = {
		itemRect.x() + 2,
		itemRect.y() + static_cast<int>(itemRect.height() / 2)};

	spk::MouseModule mouseModule;
	mouseModule.bind(&menuBar);
	EXPECT_NO_THROW({
		mouseModule.pushEvent(spk::MouseEventRecord(spk::makeEventRecord(spk::MouseMovedRecord{.position = clickPos})));
		mouseModule.pushEvent(spk::MouseEventRecord(spk::makeEventRecord(spk::MouseButtonPressedRecord{.button = spk::Mouse::Left})));
		mouseModule.pushEvent(spk::MouseEventRecord(spk::makeEventRecord(spk::MouseButtonReleasedRecord{.button = spk::Mouse::Left})));
		mouseModule.processEvents();
	});
}
