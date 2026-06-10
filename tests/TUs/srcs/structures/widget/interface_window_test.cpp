#include <gtest/gtest.h>

#include "structures/widget/spk_interface_window.hpp"
#include "structures/widget/spk_panel.hpp"
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

TEST(InterfaceWindowTest, ContentGeometryExcludesMenuAndCorners)
{
	spk::InterfaceWindow<spk::Panel> window("Window");
	window.setGeometry(spk::Rect2D(0, 0, 300, 200));

	const spk::Vector2Int cornerSize = window.backgroundFrame().cornerSize();
	const spk::Rect2D contentGeometry = window.contentObject().geometry();

	EXPECT_EQ(contentGeometry.x(), cornerSize.x);
	EXPECT_EQ(contentGeometry.y(), static_cast<int>(window.menuHeight()) + cornerSize.y);
	EXPECT_EQ(contentGeometry.width(), 300u - static_cast<unsigned int>(cornerSize.x * 2));
	EXPECT_EQ(contentGeometry.height(), 200u - window.menuHeight() - static_cast<unsigned int>(cornerSize.y * 2));
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
	window.setMenuHeight(40);

	EXPECT_EQ(window.menuHeight(), 40u);
	EXPECT_NE(window.menuHeight(), originalHeight);
}

TEST(InterfaceWindowTest, SetContentNullptrIsAllowed)
{
	spk::InterfaceWindow<spk::Panel> window("Window");

	EXPECT_NO_THROW(window.setContent(nullptr));
	EXPECT_EQ(window.content(), nullptr);
}

TEST(InterfaceWindowTest, SetContentWrongParentThrows)
{
	spk::InterfaceWindow<spk::Panel> window("Window");
	spk::Widget stranger("Stranger");

	EXPECT_THROW(window.setContent(&stranger), std::invalid_argument);
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
