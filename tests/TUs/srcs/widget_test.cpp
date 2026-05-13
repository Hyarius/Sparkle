#include <gtest/gtest.h>

#include "spk_window_modules.hpp"
#include "window_test_utils.hpp"

TEST(WidgetTest, ConstructionStoresNameAndOptionalParent)
{
	sparkle_test::RecordingWidget parent("Parent");
	sparkle_test::RecordingWidget child("Child", &parent);

	EXPECT_EQ(parent.name(), "Parent");
	EXPECT_EQ(child.name(), "Child");
	EXPECT_EQ(child.parent(), &parent);
	ASSERT_EQ(parent.nbChildren(), 1u);
	EXPECT_EQ(parent.children()[0], &child);
}

TEST(WidgetTest, SetGeometryMarksWidgetDirtyAndFirstAppendClearsIt)
{
	sparkle_test::RecordingWidget widget("Widget");
	const spk::Rect2D rect(4, 5, 100, 60);
	spk::RenderCommandBuilder builder;
	spk::RenderModule renderModule;

	widget.activate();
	widget.setGeometry(rect);

	EXPECT_TRUE(widget.isRenderCommandDirty());

	widget.appendRenderCommands(builder);
	renderModule.render(builder);

	EXPECT_EQ(widget.geometry(), rect);
	EXPECT_EQ(widget.geometryUpdateCount, 1);
	EXPECT_EQ(widget.geometrySeenDuringUpdate, rect);
	EXPECT_EQ(widget.appendRenderCommandCount, 1);
	EXPECT_EQ(widget.renderCount, 1);
	EXPECT_FALSE(widget.isRenderCommandDirty());
}

TEST(WidgetTest, RequireRenderCommandRebuildTriggersAnotherDirtyPass)
{
	sparkle_test::RecordingWidget widget("Widget");
	spk::RenderCommandBuilder builder;
	spk::RenderModule renderModule;

	widget.activate();

	widget.appendRenderCommands(builder);
	renderModule.render(builder);
	ASSERT_EQ(widget.geometryUpdateCount, 1);

	builder.clear();
	widget.requireRenderCommandRebuild();
	EXPECT_TRUE(widget.isRenderCommandDirty());

	widget.appendRenderCommands(builder);
	renderModule.render(builder);

	EXPECT_EQ(widget.geometryUpdateCount, 2);
	EXPECT_EQ(widget.renderCount, 2);
	EXPECT_FALSE(widget.isRenderCommandDirty());
}

TEST(WidgetTest, SettingSameGeometryDoesNotRequestAnotherDirtyPass)
{
	sparkle_test::RecordingWidget widget("Widget");
	const spk::Rect2D rect(1, 2, 30, 40);
	spk::RenderCommandBuilder builder;
	spk::RenderModule renderModule;

	widget.activate();

	widget.setGeometry(rect);
	widget.appendRenderCommands(builder);
	renderModule.render(builder);
	ASSERT_EQ(widget.geometryUpdateCount, 1);

	builder.clear();
	widget.setGeometry(rect);
	EXPECT_FALSE(widget.isRenderCommandDirty());

	widget.appendRenderCommands(builder);
	renderModule.render(builder);

	EXPECT_EQ(widget.geometryUpdateCount, 1);
	EXPECT_EQ(widget.renderCount, 2);
}

TEST(WidgetTest, AppendRenderCommandsVisitsSelfBeforeChildrenAndRenderExecutesInThatOrder)
{
	sparkle_test::RecordingWidget parent("Parent");
	sparkle_test::RecordingWidget child("Child", &parent);
	spk::RenderCommandBuilder builder;
	spk::RenderModule renderModule;
	std::vector<std::string> callLog;

	parent.bindSharedCallLog(&callLog);
	child.bindSharedCallLog(&callLog);
	parent.activate();
	child.activate();

	parent.appendRenderCommands(builder);
	renderModule.render(builder);

	std::vector<std::string> expected = {
		"Parent:append_render",
		"Child:append_render",
		"Parent:render",
		"Child:render"
	};

	EXPECT_EQ(callLog, expected);
}

TEST(WidgetTest, UpdateVisitsChildrenBeforeSelf)
{
	sparkle_test::RecordingWidget parent("Parent");
	sparkle_test::RecordingWidget child("Child", &parent);
	std::vector<std::string> callLog;

	parent.bindSharedCallLog(&callLog);
	child.bindSharedCallLog(&callLog);
	parent.activate();
	child.activate();

	spk::UpdateTick tick;
	parent.update(tick);

	std::vector<std::string> expected = {
		"Child:update",
		"Parent:update"
	};

	EXPECT_EQ(callLog, expected);
}

TEST(WidgetTest, FrameEventsPropagateToChildrenBeforeParent)
{
	sparkle_test::RecordingWidget parent("Parent");
	sparkle_test::RecordingWidget child("Child", &parent);
	std::vector<std::string> callLog;

	parent.bindSharedCallLog(&callLog);
	child.bindSharedCallLog(&callLog);
	parent.activate();
	child.activate();

	spk::FrameEventRecord shownEvent = spk::FrameEventRecord(spk::makeEventRecord(spk::WindowShownRecord{}));
	parent.dispatchFrameEvent(shownEvent);

	std::vector<std::string> expected = {
		"Child:frame:WindowShown",
		"Parent:frame:WindowShown"
	};

	EXPECT_EQ(callLog, expected);
	EXPECT_EQ(child.frameEventCount, 1);
	EXPECT_EQ(parent.frameEventCount, 1);
}

TEST(WidgetTest, ConsumedFrameEventStopsPropagationToParent)
{
	sparkle_test::RecordingWidget parent("Parent");
	sparkle_test::RecordingWidget child("Child", &parent);

	parent.activate();
	child.activate();
	child.consumeFrameEvent = true;

	spk::FrameEventRecord hiddenEvent = spk::FrameEventRecord(spk::makeEventRecord(spk::WindowHiddenRecord{}));
	parent.dispatchFrameEvent(hiddenEvent);

	EXPECT_EQ(child.frameEventCount, 1);
	EXPECT_EQ(parent.frameEventCount, 0);
}

TEST(WidgetTest, MouseAndKeyboardDispatchReachDedicatedHandlers)
{
	sparkle_test::RecordingWidget widget("Widget");
	widget.activate();

	spk::Mouse mouse;
	spk::Keyboard keyboard;
	spk::MouseEventRecord mouseEvent = spk::MouseEventRecord(spk::makeEventRecord(spk::MouseButtonPressedRecord{
		.button = spk::Mouse::Left}));
	spk::KeyboardEventRecord keyboardEvent = spk::KeyboardEventRecord(spk::makeEventRecord(spk::KeyPressedRecord{
		.key = spk::Keyboard::Return,
		.isRepeated = false}));

	widget.dispatchMouseEvent(mouseEvent, mouse);
	widget.dispatchKeyboardEvent(keyboardEvent, keyboard);

	EXPECT_EQ(widget.mouseEventCount, 1);
	EXPECT_EQ(widget.keyboardEventCount, 1);
	ASSERT_EQ(widget.mouseEventKinds.size(), 1u);
	EXPECT_EQ(widget.mouseEventKinds[0], "MouseButtonPressed");
	ASSERT_EQ(widget.keyboardEventKinds.size(), 1u);
	EXPECT_EQ(widget.keyboardEventKinds[0], "KeyPressed");
}

TEST(WidgetTest, DeactivatedWidgetSkipsRenderCommandAppendUpdateAndEventDispatch)
{
	sparkle_test::RecordingWidget widget("Widget");
	spk::RenderCommandBuilder builder;
	spk::RenderModule renderModule;
	spk::UpdateTick tick;

	widget.setGeometry(spk::Rect2D(1, 1, 10, 10));

	widget.appendRenderCommands(builder);
	renderModule.render(builder);
	widget.update(tick);
	spk::Mouse mouse;
	spk::Keyboard keyboard;
	spk::FrameEventRecord frameEvent = spk::FrameEventRecord(spk::makeEventRecord(spk::WindowShownRecord{}));
	spk::MouseEventRecord mouseEvent = spk::MouseEventRecord(spk::makeEventRecord(spk::MouseMovedRecord{
		.position = spk::Vector2Int(1, 2),
		.delta = spk::Vector2Int(3, 4)}));
	spk::KeyboardEventRecord keyboardEvent = spk::KeyboardEventRecord(spk::makeEventRecord(spk::TextInputRecord{
		.glyph = U'X'}));

	widget.dispatchFrameEvent(frameEvent);
	widget.dispatchMouseEvent(mouseEvent, mouse);
	widget.dispatchKeyboardEvent(keyboardEvent, keyboard);

	EXPECT_EQ(widget.geometryUpdateCount, 0);
	EXPECT_EQ(widget.appendRenderCommandCount, 0);
	EXPECT_EQ(widget.renderCount, 0);
	EXPECT_EQ(widget.updateCount, 0);
	EXPECT_EQ(widget.frameEventCount, 0);
	EXPECT_EQ(widget.mouseEventCount, 0);
	EXPECT_EQ(widget.keyboardEventCount, 0);
}
