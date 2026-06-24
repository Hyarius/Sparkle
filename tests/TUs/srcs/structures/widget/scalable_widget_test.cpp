#include <gtest/gtest.h>

#include "structures/widget/spk_scalable_widget.hpp"
#include "structures/widget/spk_text_edit.hpp"
#include "structures/application/module/spk_mouse_module.hpp"
#include "structures/system/device/window/window_test_utils.hpp"

TEST(ScalableWidgetTest, SetMinimumSizeGrowsCurrentGeometry)
{
	spk::ScalableWidget widget("Scalable");
	widget.setGeometry(spk::Rect2D(0, 0, 30, 30));

	widget.setMinimalSize({60, 40});

	EXPECT_EQ(widget.geometry().width(), 60u);
	EXPECT_EQ(widget.geometry().height(), 40u);
}

TEST(ScalableWidgetTest, DraggingRightEdgeResizesWidget)
{
	spk::ScalableWidget widget("Scalable");
	widget.activate();
	widget.setGeometry(spk::Rect2D(20, 20, 100, 100));

	spk::MouseModule mouseModule;
	mouseModule.bind(&widget);
	mouseModule.pushEvent(spk::MouseEventRecord(spk::makeEventRecord(spk::MouseMovedRecord{.position = {120, 60}})));
	mouseModule.pushEvent(spk::MouseEventRecord(spk::makeEventRecord(spk::MouseButtonPressedRecord{.button = spk::Mouse::Left})));
	mouseModule.processEvents();

	EXPECT_TRUE(widget.isResizing());

	mouseModule.pushEvent(spk::MouseEventRecord(spk::makeEventRecord(spk::MouseMovedRecord{.position = {140, 60}})));
	mouseModule.pushEvent(spk::MouseEventRecord(spk::makeEventRecord(spk::MouseButtonReleasedRecord{.button = spk::Mouse::Left})));
	mouseModule.processEvents();

	EXPECT_FALSE(widget.isResizing());
	EXPECT_EQ(widget.geometry(), spk::Rect2D(20, 20, 120, 100));
}

TEST(ScalableWidgetTest, DraggingTopLeftCornerMovesAnchorAndResizes)
{
	spk::ScalableWidget widget("Scalable");
	widget.activate();
	widget.setGeometry(spk::Rect2D(20, 20, 100, 100));

	spk::MouseModule mouseModule;
	mouseModule.bind(&widget);
	mouseModule.pushEvent(spk::MouseEventRecord(spk::makeEventRecord(spk::MouseMovedRecord{.position = {20, 20}})));
	mouseModule.pushEvent(spk::MouseEventRecord(spk::makeEventRecord(spk::MouseButtonPressedRecord{.button = spk::Mouse::Left})));
	mouseModule.pushEvent(spk::MouseEventRecord(spk::makeEventRecord(spk::MouseMovedRecord{.position = {10, 10}})));
	mouseModule.pushEvent(spk::MouseEventRecord(spk::makeEventRecord(spk::MouseButtonReleasedRecord{.button = spk::Mouse::Left})));
	mouseModule.processEvents();

	EXPECT_EQ(widget.geometry(), spk::Rect2D(10, 10, 110, 110));
}

TEST(ScalableWidgetTest, HoveringEdgeRequestsResizeCursor)
{
	spk::ScalableWidget widget("Scalable");
	widget.activate();
	widget.setGeometry(spk::Rect2D(20, 20, 100, 100));

	spk::MouseModule mouseModule;
	mouseModule.bind(&widget);
	mouseModule.pushEvent(spk::MouseEventRecord(spk::makeEventRecord(spk::MouseMovedRecord{.position = {120, 60}})));
	mouseModule.processEvents();

	EXPECT_EQ(mouseModule.mouse().requestedCursorShape, "ResizeWE");

	mouseModule.pushEvent(spk::MouseEventRecord(spk::makeEventRecord(spk::MouseMovedRecord{.position = {120, 20}})));
	mouseModule.processEvents();

	EXPECT_EQ(mouseModule.mouse().requestedCursorShape, "ResizeNESW");

	mouseModule.pushEvent(spk::MouseEventRecord(spk::makeEventRecord(spk::MouseMovedRecord{.position = {70, 70}})));
	mouseModule.processEvents();

	EXPECT_EQ(mouseModule.mouse().requestedCursorShape, "Arrow");
}

TEST(TextEditCursorTest, HoveringTextEditRequestsIBeamCursor)
{
	spk::TextEdit textEdit("TextEdit");
	textEdit.setGeometry(spk::Rect2D(0, 0, 200, 40));

	spk::MouseModule mouseModule;
	mouseModule.bind(&textEdit);
	mouseModule.pushEvent(spk::MouseEventRecord(spk::makeEventRecord(spk::MouseMovedRecord{.position = {50, 20}})));
	mouseModule.processEvents();

	EXPECT_EQ(mouseModule.mouse().requestedCursorShape, "IBeam");

	mouseModule.pushEvent(spk::MouseEventRecord(spk::makeEventRecord(spk::MouseMovedRecord{.position = {500, 500}})));
	mouseModule.processEvents();

	EXPECT_EQ(mouseModule.mouse().requestedCursorShape, "Arrow");
}
