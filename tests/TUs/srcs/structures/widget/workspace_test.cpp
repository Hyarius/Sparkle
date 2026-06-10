#include <gtest/gtest.h>

#include "structures/widget/spk_panel.hpp"
#include "structures/widget/spk_workspace.hpp"

TEST(WorkspaceTest, ConstructionActivatesMenuBarAndContent)
{
	spk::Workspace<spk::Panel> workspace("Workspace");

	EXPECT_TRUE(workspace.isActivated());
	EXPECT_TRUE(workspace.menuBar().isActivated());
	EXPECT_TRUE(workspace.content().isActivated());
}

TEST(WorkspaceTest, ContentSitsBelowMenuBar)
{
	spk::Workspace<spk::Panel> workspace("Workspace");
	workspace.setGeometry(spk::Rect2D(0, 0, 400, 300));

	EXPECT_EQ(workspace.menuBar().geometry(), spk::Rect2D(0, 0, 400, 300));
	EXPECT_EQ(
		workspace.content().geometry(),
		spk::Rect2D(0, static_cast<int>(workspace.menuBar().height()), 400, 300 - workspace.menuBar().height()));
}

TEST(WorkspaceTest, MinimalSizeIncludesMenuBarHeight)
{
	spk::Workspace<spk::Panel> workspace("Workspace");
	workspace.content().setMinimalSize({100, 50});

	const spk::Vector2UInt minimalSize = workspace.minimalSize();

	EXPECT_GE(minimalSize.x, 100u);
	EXPECT_EQ(minimalSize.y, workspace.menuBar().height() + 50u);
}

TEST(WorkspaceTest, MenuBarCanHostMenus)
{
	spk::Workspace<spk::Panel> workspace("Workspace");
	workspace.setGeometry(spk::Rect2D(0, 0, 400, 300));

	spk::MenuBar::Menu* menu = workspace.menuBar().addMenu("File");
	menu->addItem("Open", []() {});

	EXPECT_EQ(workspace.menuBar().nbMenu(), 1u);
	EXPECT_EQ(menu->nbItem(), 1u);
}
