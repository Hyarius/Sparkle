#include <gtest/gtest.h>

#include "structures/widget/spk_interface_window.hpp"
#include "structures/widget/spk_panel.hpp"
#include "structures/widget/spk_widget_visual_test_helpers.hpp"
#include "structures/application/module/spk_mouse_module.hpp"
#include "structures/system/device/window/window_test_utils.hpp"

TEST(InterfaceWindowTest, ConstructionActivatesAndOwnsTypedContent)
{
	spk::InterfaceWindow<spk::Panel> window("Window");

	EXPECT_TRUE(window.isActivated());
	EXPECT_EQ(window.content(), &window.contentObject());
	EXPECT_TRUE(window.contentObject().isActivated());
	EXPECT_FALSE(window.isMinimized());
	EXPECT_FALSE(window.isMaximized());
}

TEST(InterfaceWindowTest, SetTitleUpdatesTitleLabel)
{
	spk::InterfaceWindow<spk::Panel> window("Window");

	window.setTitle("My Window");

	EXPECT_EQ(window.menuBar().titleLabel().text(), spk::Font::textFromUTF8("My Window"));
}

TEST(InterfaceWindowTest, TitleLabelUsesCompactInterfaceWindowStyle)
{
	spk::InterfaceWindow<spk::Panel> window("Window");

	EXPECT_EQ(window.menuBar().titleLabel().textSize(), spk::Font::Size(12, 0));
	EXPECT_EQ(window.menuBar().titleLabel().padding(), spk::Vector2Int(3, 0));

	window.setGeometry(spk::Rect2D(0, 0, 300, 200));

	EXPECT_EQ(window.menuBar().titleLabel().textSize(), spk::Font::Size(12, 0));
}

TEST(InterfaceWindowTest, TitleButtonsUseCompactIconSize)
{
	spk::InterfaceWindow<spk::Panel> window("Window");
	window.setGeometry(spk::Rect2D(0, 0, 300, 200));

	const spk::PushButton& closeButton = window.menuBar().closeButton();

	ASSERT_TRUE(closeButton.iconSize().has_value());
	EXPECT_EQ(*closeButton.iconSize(), spk::Vector2UInt(12, 12));
	ASSERT_TRUE(closeButton.iconPadding().has_value());
	EXPECT_EQ(*closeButton.iconPadding(), spk::Vector2UInt(4, 4));
	EXPECT_EQ(closeButton.releasedBackground().cornerSize(), spk::Vector2Int(2, 2));
	EXPECT_EQ(closeButton.pressedBackground().cornerSize(), spk::Vector2Int(2, 2));
	EXPECT_EQ(closeButton.releasedIcon().geometry().size, spk::Vector2UInt(12, 12));
}

TEST(InterfaceWindowTest, ContentGeometryExcludesMenuAndDefaultPadding)
{
	spk::InterfaceWindow<spk::Panel> window("Window");
	window.setGeometry(spk::Rect2D(0, 0, 300, 200));

	const spk::Vector2Int cornerSize = window.backgroundFrame().cornerSize();
	const spk::IInterfaceWindow::ContentPadding padding = window.contentPadding();
	const spk::Rect2D contentGeometry = window.contentObject().geometry();

	EXPECT_EQ(padding.left, static_cast<uint32_t>(cornerSize.x + 2));
	EXPECT_EQ(padding.top, 0u);
	EXPECT_EQ(padding.right, static_cast<uint32_t>(cornerSize.y + 2));
	EXPECT_EQ(padding.bottom, static_cast<uint32_t>(cornerSize.y + 2));
	EXPECT_EQ(contentGeometry.x(), static_cast<int>(padding.left));
	EXPECT_EQ(contentGeometry.y(), static_cast<int>(window.menuHeight() + padding.top));
	EXPECT_EQ(contentGeometry.width(), 300u - padding.left - padding.right);
	EXPECT_EQ(contentGeometry.height(), 200u - window.menuHeight() - padding.top - padding.bottom);
}

TEST(InterfaceWindowTest, DefaultContentPaddingUsesMatchingCornerAxis)
{
	spk::InterfaceWindow<spk::Panel> window("Window");
	window.backgroundFrame().setCornerSize({12, 8});
	window.setGeometry(spk::Rect2D(0, 0, 300, 200));

	const spk::IInterfaceWindow::ContentPadding padding = window.contentPadding();
	const spk::Rect2D contentGeometry = window.contentObject().geometry();

	EXPECT_EQ(padding.left, 14u);
	EXPECT_EQ(padding.top, 0u);
	EXPECT_EQ(padding.right, 14u);
	EXPECT_EQ(padding.bottom, 10u);
	EXPECT_EQ(contentGeometry.x(), 14);
	EXPECT_EQ(contentGeometry.width(), 300u - 14u - 14u);
}

TEST(InterfaceWindowTest, ContentPaddingCanBeConfiguredPerSide)
{
	spk::InterfaceWindow<spk::Panel> window("Window");
	window.setContentPadding({4, 1, 12, 9});
	window.setGeometry(spk::Rect2D(0, 0, 300, 200));

	const spk::Rect2D contentGeometry = window.contentObject().geometry();

	EXPECT_EQ(window.contentPadding(), (spk::IInterfaceWindow::ContentPadding{4, 1, 12, 9}));
	EXPECT_EQ(contentGeometry.x(), 4);
	EXPECT_EQ(contentGeometry.y(), static_cast<int>(window.menuHeight() + 1));
	EXPECT_EQ(contentGeometry.width(), 300u - 4u - 12u);
	EXPECT_EQ(contentGeometry.height(), 200u - window.menuHeight() - 1u - 9u);
	EXPECT_EQ(window.contentSize(), spk::Vector2UInt(300u - 4u - 12u, 200u - window.menuHeight() - 1u - 9u));
}

TEST(InterfaceWindowTest, ResetContentPaddingRestoresDefault)
{
	spk::InterfaceWindow<spk::Panel> window("Window");
	window.setContentPadding({4, 1, 12, 9});
	window.resetContentPadding();
	window.setGeometry(spk::Rect2D(0, 0, 300, 200));

	const spk::Vector2Int cornerSize = window.backgroundFrame().cornerSize();

	EXPECT_EQ(window.contentPadding(), (spk::IInterfaceWindow::ContentPadding{
		static_cast<uint32_t>(cornerSize.x + 2),
		0,
		static_cast<uint32_t>(cornerSize.y + 2),
		static_cast<uint32_t>(cornerSize.y + 2)}));
	EXPECT_EQ(window.contentObject().geometry().anchor, spk::Vector2Int(cornerSize.x + 2, static_cast<int>(window.menuHeight())));
}

TEST(InterfaceWindowTest, MinimizeTogglesBackgroundFrames)
{
	spk::InterfaceWindow<spk::Panel> window("Window");
	window.setGeometry(spk::Rect2D(0, 0, 300, 200));

	window.minimize();

	EXPECT_TRUE(window.isMinimized());
	EXPECT_FALSE(window.backgroundFrame().isActivated());
	EXPECT_TRUE(window.minimizedBackgroundFrame().isActivated());

	window.minimize();

	EXPECT_FALSE(window.isMinimized());
	EXPECT_TRUE(window.backgroundFrame().isActivated());
	EXPECT_FALSE(window.minimizedBackgroundFrame().isActivated());
}

TEST(InterfaceWindowTest, MaximizeUsesParentSizeAndRestores)
{
	spk::Widget root("Root");
	root.setGeometry(spk::Rect2D(0, 0, 800, 600));
	root.activate();

	spk::InterfaceWindow<spk::Panel> window("Window", &root);
	window.setGeometry(spk::Rect2D(50, 50, 300, 200));

	window.maximize();

	EXPECT_TRUE(window.isMaximized());
	EXPECT_EQ(window.geometry(), spk::Rect2D(0, 0, 800, 600));

	window.maximize();

	EXPECT_FALSE(window.isMaximized());
	EXPECT_EQ(window.geometry(), spk::Rect2D(50, 50, 300, 200));
}

TEST(InterfaceWindowTest, CloseDeactivatesWindow)
{
	spk::InterfaceWindow<spk::Panel> window("Window");

	window.close();

	EXPECT_FALSE(window.isActivated());
}

TEST(InterfaceWindowTest, DraggingTitleBarMovesWindow)
{
	spk::InterfaceWindow<spk::Panel> window("Window");
	window.setGeometry(spk::Rect2D(10, 10, 300, 200));

	spk::MouseModule mouseModule;
	mouseModule.bind(&window);
	mouseModule.pushEvent(spk::MouseEventRecord(spk::makeEventRecord(spk::MouseMovedRecord{.position = {60, 20}})));
	mouseModule.pushEvent(spk::MouseEventRecord(spk::makeEventRecord(spk::MouseButtonPressedRecord{.button = spk::Mouse::Left})));
	mouseModule.processEvents();

	EXPECT_TRUE(window.isMoving());

	mouseModule.pushEvent(spk::MouseEventRecord(spk::makeEventRecord(spk::MouseMovedRecord{.position = {110, 60}})));
	mouseModule.pushEvent(spk::MouseEventRecord(spk::makeEventRecord(spk::MouseButtonReleasedRecord{.button = spk::Mouse::Left})));
	mouseModule.processEvents();

	EXPECT_FALSE(window.isMoving());
	EXPECT_EQ(window.geometry().anchor, spk::Vector2Int(60, 50));
	EXPECT_EQ(window.geometry().size, spk::Vector2UInt(300, 200));
}

TEST(InterfaceWindowTest, SubscribeToCloseTriggersOnButtonClick)
{
	spk::InterfaceWindow<spk::Panel> window("Window");
	window.setGeometry(spk::Rect2D(0, 0, 300, 200));

	int closeCount = 0;
	auto contract = window.subscribeTo(spk::IInterfaceWindow::Event::Close, [&closeCount]()
	{
		++closeCount;
	});

	const spk::Rect2D closeRect = window.menuBar().closeButton().viewport().geometry();
	const spk::Vector2Int clickPosition = {
		closeRect.x() + static_cast<int>(closeRect.width() / 2),
		closeRect.y() + static_cast<int>(closeRect.height() / 2)};

	spk::MouseModule mouseModule;
	mouseModule.bind(&window);
	mouseModule.pushEvent(spk::MouseEventRecord(spk::makeEventRecord(spk::MouseMovedRecord{.position = clickPosition})));
	mouseModule.pushEvent(spk::MouseEventRecord(spk::makeEventRecord(spk::MouseButtonPressedRecord{.button = spk::Mouse::Left})));
	mouseModule.pushEvent(spk::MouseEventRecord(spk::makeEventRecord(spk::MouseButtonReleasedRecord{.button = spk::Mouse::Left})));
	mouseModule.processEvents();

	EXPECT_EQ(closeCount, 1);
}

TEST(InterfaceWindowTest, DeactivateMenuButtonHidesIt)
{
	spk::InterfaceWindow<spk::Panel> window("Window");
	window.setGeometry(spk::Rect2D(0, 0, 300, 200));

	EXPECT_TRUE(window.menuBar().minimizeButton().isActivated());

	window.deactivateMenuButton(spk::IInterfaceWindow::MenuBar::Button::Minimize);

	EXPECT_FALSE(window.menuBar().minimizeButton().isActivated());

	window.activateMenuButton(spk::IInterfaceWindow::MenuBar::Button::Minimize);

	EXPECT_TRUE(window.menuBar().minimizeButton().isActivated());
}

TEST(InterfaceWindowTest, SubscribeToMinimizeTriggersOnButtonClick)
{
	spk::InterfaceWindow<spk::Panel> window("Window");
	window.setGeometry(spk::Rect2D(0, 0, 300, 200));

	int count = 0;
	auto contract = window.subscribeTo(spk::IInterfaceWindow::Event::Minimize, [&count]() { ++count; });

	const spk::Rect2D btnRect = window.menuBar().minimizeButton().viewport().geometry();
	const spk::Vector2Int pos = {btnRect.x() + static_cast<int>(btnRect.width() / 2),
	                              btnRect.y() + static_cast<int>(btnRect.height() / 2)};

	spk::MouseModule mouseModule;
	mouseModule.bind(&window);
	mouseModule.pushEvent(spk::MouseEventRecord(spk::makeEventRecord(spk::MouseMovedRecord{.position = pos})));
	mouseModule.pushEvent(spk::MouseEventRecord(spk::makeEventRecord(spk::MouseButtonPressedRecord{.button = spk::Mouse::Left})));
	mouseModule.pushEvent(spk::MouseEventRecord(spk::makeEventRecord(spk::MouseButtonReleasedRecord{.button = spk::Mouse::Left})));
	mouseModule.processEvents();

	EXPECT_EQ(count, 1);
}

TEST(InterfaceWindowTest, SubscribeToMaximizeTriggersOnButtonClick)
{
	spk::Widget root("Root");
	root.setGeometry(spk::Rect2D(0, 0, 800, 600));
	root.activate();

	spk::InterfaceWindow<spk::Panel> window("Window", &root);
	window.setGeometry(spk::Rect2D(0, 0, 300, 200));

	int count = 0;
	auto contract = window.subscribeTo(spk::IInterfaceWindow::Event::Maximize, [&count]() { ++count; });

	const spk::Rect2D btnRect = window.menuBar().maximizeButton().viewport().geometry();
	const spk::Vector2Int pos = {btnRect.x() + static_cast<int>(btnRect.width() / 2),
	                              btnRect.y() + static_cast<int>(btnRect.height() / 2)};

	spk::MouseModule mouseModule;
	mouseModule.bind(&window);
	mouseModule.pushEvent(spk::MouseEventRecord(spk::makeEventRecord(spk::MouseMovedRecord{.position = pos})));
	mouseModule.pushEvent(spk::MouseEventRecord(spk::makeEventRecord(spk::MouseButtonPressedRecord{.button = spk::Mouse::Left})));
	mouseModule.pushEvent(spk::MouseEventRecord(spk::makeEventRecord(spk::MouseButtonReleasedRecord{.button = spk::Mouse::Left})));
	mouseModule.processEvents();

	EXPECT_EQ(count, 1);
}

TEST(InterfaceWindowTest, MaximizeWithNoParentIsNoOp)
{
	spk::InterfaceWindow<spk::Panel> window("Window");
	window.setGeometry(spk::Rect2D(50, 50, 300, 200));

	window.maximize();

	EXPECT_FALSE(window.isMaximized());
	EXPECT_EQ(window.geometry(), spk::Rect2D(50, 50, 300, 200));
}

TEST(InterfaceWindowTest, MinimizeWhileMaximizedRestoresFirst)
{
	spk::Widget root("Root");
	root.setGeometry(spk::Rect2D(0, 0, 800, 600));
	root.activate();

	spk::InterfaceWindow<spk::Panel> window("Window", &root);
	window.setGeometry(spk::Rect2D(50, 50, 300, 200));

	window.maximize();
	ASSERT_TRUE(window.isMaximized());

	window.minimize();

	EXPECT_FALSE(window.isMaximized());
	EXPECT_TRUE(window.isMinimized());
}

TEST(InterfaceWindowTest, SetMenuHeightUpdatesLayout)
{
	spk::InterfaceWindow<spk::Panel> window("Window");
	window.setGeometry(spk::Rect2D(0, 0, 300, 200));

	const unsigned int originalHeight = window.menuHeight();
	window.setMenuHeight(originalHeight + 10);

	EXPECT_EQ(window.menuHeight(), originalHeight + 10);
	EXPECT_NE(window.menuHeight(), originalHeight);

	window.setMenuHeight(1);

	EXPECT_EQ(window.menuHeight(), originalHeight);
}

TEST(InterfaceWindowTest, SetContentNullptrIsAllowed)
{
	spk::InterfaceWindow<spk::Panel> window("Window");
	spk::IInterfaceWindow& interfaceWindow = window;

	EXPECT_NO_THROW(interfaceWindow.setContent(nullptr));
	EXPECT_EQ(window.content(), nullptr);
}

TEST(InterfaceWindowTest, SetContentWrongParentThrows)
{
	spk::InterfaceWindow<spk::Panel> window("Window");
	spk::IInterfaceWindow& interfaceWindow = window;
	spk::Widget stranger("Stranger");

	EXPECT_THROW(interfaceWindow.setContent(&stranger), std::invalid_argument);
}

TEST(InterfaceWindowTest, ConstAccessorsReturnSameObjects)
{
	spk::InterfaceWindow<spk::Panel> window("Window");
	const spk::IInterfaceWindow& cw = window;

	EXPECT_EQ(&cw.menuBar(), &window.menuBar());
	EXPECT_EQ(&cw.backgroundFrame(), &window.backgroundFrame());
	EXPECT_EQ(&cw.minimizedBackgroundFrame(), &window.minimizedBackgroundFrame());
	EXPECT_EQ(cw.content(), window.content());
}

TEST(InterfaceWindowTest, SubscribeOnResizeFiresOnGeometryChange)
{
	spk::InterfaceWindow<spk::Panel> window("Window");
	window.setGeometry(spk::Rect2D(0, 0, 300, 200));

	spk::Vector2UInt lastSize = {0, 0};
	auto contract = window.subscribeOnResize([&lastSize](const spk::Vector2UInt& p_size) { lastSize = p_size; });

	window.setGeometry(spk::Rect2D(0, 0, 400, 300));

	EXPECT_GT(lastSize.x, 0u);
	EXPECT_GT(lastSize.y, 0u);
}

TEST(InterfaceWindowTest, IsMovingGetterReflectsState)
{
	spk::InterfaceWindow<spk::Panel> window("Window");
	window.setGeometry(spk::Rect2D(10, 10, 300, 200));

	EXPECT_FALSE(window.isMoving());

	spk::MouseModule mouseModule;
	mouseModule.bind(&window);
	mouseModule.pushEvent(spk::MouseEventRecord(spk::makeEventRecord(spk::MouseMovedRecord{.position = {60, 20}})));
	mouseModule.pushEvent(spk::MouseEventRecord(spk::makeEventRecord(spk::MouseButtonPressedRecord{.button = spk::Mouse::Left})));
	mouseModule.processEvents();

	EXPECT_TRUE(window.isMoving());
}

TEST(InterfaceWindowVisualTest, RendersDefault)
{
	const spk::Rect2D captureRect(0, 0, 320, 200);

	spk::InterfaceWindow<spk::Panel> window("Window");
	window.setTitle("My Window");

	const sparkle_test::ImageComparisonResult result =
		spk::test::compareSnapshot(window, "InterfaceWindowVisual", "default", captureRect);

	EXPECT_TRUE(result.matches);
}

TEST(InterfaceWindowVisualTest, RendersMinimized)
{
	const spk::Rect2D captureRect(0, 0, 320, 200);

	spk::InterfaceWindow<spk::Panel> window("Window");
	window.setTitle("Minimized Window");
	window.setGeometry(captureRect.atOrigin());
	window.minimize();

	const sparkle_test::ImageComparisonResult result =
		spk::test::compareSnapshot(window, "InterfaceWindowVisual", "minimized", captureRect);

	EXPECT_TRUE(result.matches);
}

TEST(InterfaceWindowVisualTest, RendersWithoutMinimizeButton)
{
	const spk::Rect2D captureRect(0, 0, 320, 200);

	spk::InterfaceWindow<spk::Panel> window("Window");
	window.setTitle("No Minimize");
	window.deactivateMenuButton(spk::IInterfaceWindow::MenuBar::Button::Minimize);

	const sparkle_test::ImageComparisonResult result =
		spk::test::compareSnapshot(window, "InterfaceWindowVisual", "no_minimize_button", captureRect);

	EXPECT_TRUE(result.matches);
}
