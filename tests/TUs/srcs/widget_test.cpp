#include <gtest/gtest.h>

#include <memory>
#include <utility>
#include <vector>

#include "spk_render_module.hpp"
#include "window_test_utils.hpp"

namespace
{
	void publishAndRender(spk::RenderModule& p_module, spk::RenderSnapshot p_snapshot)
	{
		sparkle_test::TestRenderContext renderContext(std::make_shared<sparkle_test::TestSurfaceState>());

		p_module.publishSnapshot(std::make_shared<spk::RenderSnapshot>(std::move(p_snapshot)));
		p_module.render(renderContext);
	}
}

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
	spk::RenderSnapshotBuilder builder;
	spk::RenderModule renderModule;

	widget.activate();
	widget.setGeometry(rect);

	EXPECT_TRUE(widget.isRenderCommandDirty());

	widget.appendRenderUnits(builder);
	publishAndRender(renderModule, builder.build());

	EXPECT_EQ(widget.geometry(), rect);
	EXPECT_EQ(widget.geometryUpdateCount, 1);
	EXPECT_EQ(widget.geometrySeenDuringUpdate, rect);
	EXPECT_EQ(widget.appendRenderCommandCount, 1);
	EXPECT_EQ(widget.renderCount, 1);
	EXPECT_FALSE(widget.isRenderCommandDirty());
}

TEST(WidgetTest, InvalidateRenderUnitTriggersAnotherDirtyPass)
{
	sparkle_test::RecordingWidget widget("Widget");
	spk::RenderSnapshotBuilder builder;
	spk::RenderModule renderModule;

	widget.activate();

	widget.appendRenderUnits(builder);
	publishAndRender(renderModule, builder.build());
	ASSERT_EQ(widget.geometryUpdateCount, 1);

	builder.clear();
	widget.invalidateRenderUnit();
	EXPECT_TRUE(widget.isRenderCommandDirty());

	widget.appendRenderUnits(builder);
	publishAndRender(renderModule, builder.build());

	EXPECT_EQ(widget.geometryUpdateCount, 2);
	EXPECT_EQ(widget.renderCount, 2);
	EXPECT_FALSE(widget.isRenderCommandDirty());
}

TEST(WidgetTest, AppendRenderUnitsReusesCleanUnitsAndRebuildsOnlyDirtyWidgets)
{
	sparkle_test::RecordingWidget parent("Parent");
	sparkle_test::RecordingWidget child("Child", &parent);
	spk::RenderSnapshotBuilder builder;
	spk::RenderModule renderModule;

	parent.activate();
	child.activate();

	parent.appendRenderUnits(builder);
	publishAndRender(renderModule, builder.build());

	EXPECT_EQ(parent.appendRenderCommandCount, 1);
	EXPECT_EQ(child.appendRenderCommandCount, 1);
	EXPECT_EQ(parent.renderCount, 1);
	EXPECT_EQ(child.renderCount, 1);

	builder.clear();
	parent.appendRenderUnits(builder);
	publishAndRender(renderModule, builder.build());

	EXPECT_EQ(parent.appendRenderCommandCount, 1);
	EXPECT_EQ(child.appendRenderCommandCount, 1);
	EXPECT_EQ(parent.renderCount, 2);
	EXPECT_EQ(child.renderCount, 2);

	builder.clear();
	child.invalidateRenderUnit();
	parent.appendRenderUnits(builder);
	publishAndRender(renderModule, builder.build());

	EXPECT_EQ(parent.appendRenderCommandCount, 1);
	EXPECT_EQ(child.appendRenderCommandCount, 2);
	EXPECT_EQ(parent.renderCount, 3);
	EXPECT_EQ(child.renderCount, 3);
}

TEST(WidgetTest, OldSnapshotsKeepTheirPreviousRenderUnitsAfterWidgetRebuilds)
{
	sparkle_test::CallbackWidget widget("Widget");
	spk::RenderSnapshotBuilder firstBuilder;
	spk::RenderSnapshotBuilder secondBuilder;
	spk::RenderModule renderModule;
	std::vector<int> calls;

	widget.activate();
	widget.onExecuteRenderCommand = [&calls]() { calls.push_back(1); };
	widget.appendRenderUnits(firstBuilder);
	std::shared_ptr<spk::RenderSnapshot> firstSnapshot =
		std::make_shared<spk::RenderSnapshot>(firstBuilder.build());

	widget.onExecuteRenderCommand = [&calls]() { calls.push_back(2); };
	widget.invalidateRenderUnit();
	widget.appendRenderUnits(secondBuilder);
	std::shared_ptr<spk::RenderSnapshot> secondSnapshot =
		std::make_shared<spk::RenderSnapshot>(secondBuilder.build());
	sparkle_test::TestRenderContext renderContext(std::make_shared<sparkle_test::TestSurfaceState>());

	renderModule.publishSnapshot(firstSnapshot);
	renderModule.render(renderContext);
	renderModule.publishSnapshot(secondSnapshot);
	renderModule.render(renderContext);

	EXPECT_EQ(calls, std::vector<int>({1, 2}));
}

TEST(WidgetTest, SettingSameGeometryDoesNotRequestAnotherDirtyPass)
{
	sparkle_test::RecordingWidget widget("Widget");
	const spk::Rect2D rect(1, 2, 30, 40);
	spk::RenderSnapshotBuilder builder;
	spk::RenderModule renderModule;

	widget.activate();

	widget.setGeometry(rect);
	widget.appendRenderUnits(builder);
	publishAndRender(renderModule, builder.build());
	ASSERT_EQ(widget.geometryUpdateCount, 1);

	builder.clear();
	widget.setGeometry(rect);
	EXPECT_FALSE(widget.isRenderCommandDirty());

	widget.appendRenderUnits(builder);
	publishAndRender(renderModule, builder.build());

	EXPECT_EQ(widget.geometryUpdateCount, 1);
	EXPECT_EQ(widget.renderCount, 2);
}

TEST(WidgetTest, AppendRenderUnitsVisitsSelfBeforeChildrenAndRenderExecutesInThatOrder)
{
	sparkle_test::RecordingWidget parent("Parent");
	sparkle_test::RecordingWidget child("Child", &parent);
	spk::RenderSnapshotBuilder builder;
	spk::RenderModule renderModule;
	std::vector<std::string> callLog;

	parent.bindSharedCallLog(&callLog);
	child.bindSharedCallLog(&callLog);
	parent.activate();
	child.activate();

	parent.appendRenderUnits(builder);
	publishAndRender(renderModule, builder.build());

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
	spk::RenderSnapshotBuilder builder;
	spk::RenderModule renderModule;
	spk::UpdateTick tick;

	widget.setGeometry(spk::Rect2D(1, 1, 10, 10));

	widget.appendRenderUnits(builder);
	publishAndRender(renderModule, builder.build());
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
