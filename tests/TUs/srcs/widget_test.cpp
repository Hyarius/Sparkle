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

	parent.dispatchFrameEvent(spk::Event(spk::WindowShownPayload{}));

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

	parent.dispatchFrameEvent(spk::Event(spk::WindowHiddenPayload{}));

	EXPECT_EQ(child.frameEventCount, 1);
	EXPECT_EQ(parent.frameEventCount, 0);
}

TEST(WidgetTest, MouseAndKeyboardDispatchReachDedicatedHandlers)
{
	sparkle_test::RecordingWidget widget("Widget");
	widget.activate();

	widget.dispatchMouseEvent(spk::Event(spk::MouseButtonPressedPayload{
		.button = spk::Mouse::Left}));
	widget.dispatchKeyboardEvent(spk::Event(spk::KeyPressedPayload{
		.key = spk::Keyboard::Return,
		.isRepeated = false}));

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
	widget.dispatchFrameEvent(spk::Event(spk::WindowShownPayload{}));
	widget.dispatchMouseEvent(spk::Event(spk::MouseMovedPayload{
		.position = spk::Vector2Int(1, 2),
		.delta = spk::Vector2Int(3, 4)}));
	widget.dispatchKeyboardEvent(spk::Event(spk::TextInputPayload{
		.glyph = U'X'}));

	EXPECT_EQ(widget.geometryUpdateCount, 0);
	EXPECT_EQ(widget.appendRenderCommandCount, 0);
	EXPECT_EQ(widget.renderCount, 0);
	EXPECT_EQ(widget.updateCount, 0);
	EXPECT_EQ(widget.frameEventCount, 0);
	EXPECT_EQ(widget.mouseEventCount, 0);
	EXPECT_EQ(widget.keyboardEventCount, 0);
}