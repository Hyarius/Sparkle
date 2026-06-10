#include <gtest/gtest.h>

#include <stdexcept>

#include "structures/widget/spk_panel.hpp"
#include "structures/widget/spk_scroll_area.hpp"
#include "structures/system/device/window/window_test_utils.hpp"

TEST(ContainerWidgetTest, SetContentRejectsNonChildWidget)
{
	spk::ContainerWidget container("Container");
	spk::Widget stranger("Stranger");

	EXPECT_THROW(container.setContent(&stranger), std::invalid_argument);
}

TEST(ContainerWidgetTest, ContentFollowsAnchorAndSize)
{
	spk::ContainerWidget container("Container");
	spk::Widget content("Content", &container);

	container.setContent(&content);
	container.setContentSize({200, 300});
	container.setContentAnchor({-50, -60});

	EXPECT_EQ(content.geometry(), spk::Rect2D(-50, -60, 200, 300));
	EXPECT_EQ(container.content(), &content);
}

TEST(ScrollAreaTest, OwnsTypedContent)
{
	spk::ScrollArea<spk::Panel> scrollArea("ScrollArea");

	EXPECT_TRUE(scrollArea.isActivated());
	EXPECT_EQ(scrollArea.content(), &scrollArea.contentObject());
	EXPECT_TRUE(scrollArea.contentObject().isActivated());
}

TEST(ScrollAreaTest, ViewSizeExcludesScrollBars)
{
	spk::ScrollArea<spk::Panel> scrollArea("ScrollArea");
	scrollArea.setGeometry(spk::Rect2D(0, 0, 116, 116));

	EXPECT_EQ(scrollArea.scrollBarWidth(), 16u);
	EXPECT_EQ(scrollArea.viewSize(), spk::Vector2UInt(100, 100));

	scrollArea.setScrollBarVisible(spk::Orientation::Horizontal, false);
	EXPECT_EQ(scrollArea.viewSize(), spk::Vector2UInt(100, 116));

	scrollArea.setScrollBarVisible(spk::Orientation::Vertical, false);
	EXPECT_EQ(scrollArea.viewSize(), spk::Vector2UInt(116, 116));
}

TEST(ScrollAreaTest, VerticalScrollMovesContentAnchor)
{
	spk::ScrollArea<spk::Panel> scrollArea("ScrollArea");
	scrollArea.setGeometry(spk::Rect2D(0, 0, 116, 116));
	scrollArea.setContentSize({100, 300});

	EXPECT_EQ(scrollArea.container().contentAnchor(), spk::Vector2Int(0, 0));

	scrollArea.verticalScrollBar().setRatio(1.0f);

	EXPECT_EQ(scrollArea.container().contentAnchor(), spk::Vector2Int(0, -200));

	scrollArea.verticalScrollBar().setRatio(0.5f);

	EXPECT_EQ(scrollArea.container().contentAnchor(), spk::Vector2Int(0, -100));
}

TEST(ScrollAreaTest, ContentSmallerThanViewDoesNotScroll)
{
	spk::ScrollArea<spk::Panel> scrollArea("ScrollArea");
	scrollArea.setGeometry(spk::Rect2D(0, 0, 116, 116));
	scrollArea.setContentSize({50, 50});

	scrollArea.verticalScrollBar().setRatio(1.0f);

	EXPECT_EQ(scrollArea.container().contentAnchor(), spk::Vector2Int(0, 0));
}

TEST(ScrollAreaTest, ScrollBarScalesMatchVisibleRatio)
{
	spk::ScrollArea<spk::Panel> scrollArea("ScrollArea");
	scrollArea.setGeometry(spk::Rect2D(0, 0, 116, 116));
	scrollArea.setContentSize({100, 200});

	EXPECT_FLOAT_EQ(scrollArea.verticalScrollBar().sliderBar().scale(), 0.5f);
	EXPECT_FLOAT_EQ(scrollArea.horizontalScrollBar().sliderBar().scale(), 1.0f);
}

TEST(ScrollAreaTest, MouseWheelScrollsVertically)
{
	spk::ScrollArea<spk::Panel> scrollArea("ScrollArea");
	scrollArea.setGeometry(spk::Rect2D(0, 0, 116, 116));
	scrollArea.setContentSize({100, 300});
	scrollArea.verticalScrollBar().setRatio(0.5f);

	spk::MouseModule mouseModule;
	mouseModule.bind(&scrollArea);
	mouseModule.pushEvent(spk::MouseEventRecord(spk::makeEventRecord(spk::MouseMovedRecord{.position = {50, 50}})));
	mouseModule.pushEvent(spk::MouseEventRecord(spk::makeEventRecord(spk::MouseWheelScrolledRecord{.delta = {0.0f, -1.0f}})));
	mouseModule.processEvents();

	EXPECT_GT(scrollArea.verticalScrollBar().ratio(), 0.5f);
}

TEST(ScrollAreaTest, SetScrollBarWidthUpdatesViewSize)
{
	spk::ScrollArea<spk::Panel> scrollArea("ScrollArea");
	scrollArea.setGeometry(spk::Rect2D(0, 0, 116, 116));

	scrollArea.setScrollBarWidth(8);

	EXPECT_EQ(scrollArea.scrollBarWidth(), 8u);
	EXPECT_EQ(scrollArea.viewSize(), spk::Vector2UInt(108, 108));
}

TEST(ScrollAreaTest, SetScrollBarWidthNoOpWhenSame)
{
	spk::ScrollArea<spk::Panel> scrollArea("ScrollArea");
	scrollArea.setGeometry(spk::Rect2D(0, 0, 116, 116));

	scrollArea.setScrollBarWidth(scrollArea.scrollBarWidth());

	EXPECT_EQ(scrollArea.viewSize(), spk::Vector2UInt(100, 100));
}

TEST(ScrollAreaTest, SetContentAssignsWidget)
{
	spk::IScrollArea scrollArea("ScrollArea");
	spk::Widget content("Content", &scrollArea.container());

	scrollArea.setContent(&content);

	EXPECT_EQ(scrollArea.content(), &content);
}

TEST(ScrollAreaTest, ConstAccessorsReturnSameObjects)
{
	spk::ScrollArea<spk::Panel> scrollArea("ScrollArea");
	const spk::IScrollArea& csa = scrollArea;

	EXPECT_EQ(csa.content(), scrollArea.content());
	EXPECT_EQ(&csa.container(), &scrollArea.container());
	EXPECT_EQ(&csa.horizontalScrollBar(), &scrollArea.horizontalScrollBar());
	EXPECT_EQ(&csa.verticalScrollBar(), &scrollArea.verticalScrollBar());
}

TEST(ScrollAreaTest, ContentSizeGetterMatchesSet)
{
	spk::ScrollArea<spk::Panel> scrollArea("ScrollArea");
	scrollArea.setGeometry(spk::Rect2D(0, 0, 116, 116));
	scrollArea.setContentSize({250, 400});

	EXPECT_EQ(scrollArea.contentSize(), spk::Vector2UInt(250, 400));
}

TEST(ScrollAreaTest, MouseWheelOutsideBoundsIsNoOp)
{
	spk::ScrollArea<spk::Panel> scrollArea("ScrollArea");
	scrollArea.setGeometry(spk::Rect2D(0, 0, 116, 116));
	scrollArea.setContentSize({100, 300});
	scrollArea.verticalScrollBar().setRatio(0.5f);
	const float ratioBefore = scrollArea.verticalScrollBar().ratio();

	spk::MouseModule mouseModule;
	mouseModule.bind(&scrollArea);
	mouseModule.pushEvent(spk::MouseEventRecord(spk::makeEventRecord(spk::MouseMovedRecord{.position = {200, 200}})));
	mouseModule.pushEvent(spk::MouseEventRecord(spk::makeEventRecord(spk::MouseWheelScrolledRecord{.delta = {0.0f, -1.0f}})));
	mouseModule.processEvents();

	EXPECT_FLOAT_EQ(scrollArea.verticalScrollBar().ratio(), ratioBefore);
}

TEST(ScrollAreaTest, HorizontalScrollMovesContentAnchor)
{
	spk::ScrollArea<spk::Panel> scrollArea("ScrollArea");
	scrollArea.setGeometry(spk::Rect2D(0, 0, 116, 116));
	scrollArea.setContentSize({300, 100});

	scrollArea.horizontalScrollBar().setRatio(1.0f);

	EXPECT_EQ(scrollArea.container().contentAnchor(), spk::Vector2Int(-200, 0));
}
