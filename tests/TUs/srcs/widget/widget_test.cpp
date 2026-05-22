#include <gtest/gtest.h>

#include <memory>
#include <utility>
#include <vector>

#include "application/module/spk_render_module.hpp"
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

TEST(WidgetTest, ChildGeometryIsStoredRelativeAndRenderedAbsolute)
{
	sparkle_test::RecordingWidget parent("Parent");
	sparkle_test::RecordingWidget child("Child", &parent);

	parent.activate();
	child.activate();

	parent.setGeometry(spk::Rect2D(10, 20, 100, 80));
	child.setGeometry(spk::Rect2D(5, 6, 30, 20));

	EXPECT_EQ(parent.geometry(), spk::Rect2D(10, 20, 100, 80));
	EXPECT_EQ(parent.absoluteGeometryForTest().anchor, spk::Vector2Int(10, 20));
	EXPECT_EQ(parent.absoluteGeometryForTest(), spk::Rect2D(10, 20, 100, 80));
	EXPECT_EQ(parent.scissorForTest(), spk::Rect2D(10, 20, 100, 80));

	EXPECT_EQ(child.geometry(), spk::Rect2D(5, 6, 30, 20));
	EXPECT_EQ(child.absoluteGeometryForTest().anchor, spk::Vector2Int(15, 26));
	EXPECT_EQ(child.absoluteGeometryForTest(), spk::Rect2D(15, 26, 30, 20));
	EXPECT_EQ(child.scissorForTest(), spk::Rect2D(15, 26, 30, 20));
}

TEST(WidgetTest, ChildScissorIsClippedByParentScissorAndUpdatesWhenParentMoves)
{
	sparkle_test::RecordingWidget parent("Parent");
	sparkle_test::RecordingWidget child("Child", &parent);

	parent.activate();
	child.activate();

	parent.setGeometry(spk::Rect2D(10, 20, 20, 20));
	child.setGeometry(spk::Rect2D(15, 15, 20, 20));

	EXPECT_EQ(child.geometry(), spk::Rect2D(15, 15, 20, 20));
	EXPECT_EQ(child.absoluteGeometryForTest().anchor, spk::Vector2Int(25, 35));
	EXPECT_EQ(child.absoluteGeometryForTest(), spk::Rect2D(25, 35, 20, 20));
	EXPECT_EQ(child.scissorForTest(), spk::Rect2D(25, 35, 5, 5));

	parent.setGeometry(spk::Rect2D(0, 0, 20, 20));

	EXPECT_EQ(child.geometry(), spk::Rect2D(15, 15, 20, 20));
	EXPECT_EQ(child.absoluteGeometryForTest().anchor, spk::Vector2Int(15, 15));
	EXPECT_EQ(child.absoluteGeometryForTest(), spk::Rect2D(15, 15, 20, 20));
	EXPECT_EQ(child.scissorForTest(), spk::Rect2D(15, 15, 5, 5));
}

TEST(WidgetTest, ParentMoveRefreshesChildAbsolutePlacementWithoutDirtyingChildRenderUnit)
{
	sparkle_test::RecordingWidget parent("Parent");
	sparkle_test::RecordingWidget child("Child", &parent);
	spk::RenderSnapshotBuilder builder;

	parent.activate();
	child.activate();

	parent.setGeometry(spk::Rect2D(10, 20, 20, 20));
	child.setGeometry(spk::Rect2D(1, 2, 3, 4));
	child.appendRenderUnits(builder);
	ASSERT_FALSE(child.isRenderCommandDirty());

	parent.setGeometry(spk::Rect2D(30, 40, 20, 20));

	EXPECT_FALSE(child.isRenderCommandDirty());
	EXPECT_EQ(child.absoluteGeometryForTest().anchor, spk::Vector2Int(31, 42));
	EXPECT_EQ(child.absoluteGeometryForTest(), spk::Rect2D(31, 42, 3, 4));
	EXPECT_EQ(child.scissorForTest(), spk::Rect2D(31, 42, 3, 4));
	EXPECT_FALSE(child.isRenderCommandDirty());
}

TEST(WidgetTest, ReparentingRefreshesAbsoluteGeometryAndScissorCache)
{
	sparkle_test::RecordingWidget firstParent("FirstParent");
	sparkle_test::RecordingWidget secondParent("SecondParent");
	sparkle_test::RecordingWidget child("Child", &firstParent);

	firstParent.setGeometry(spk::Rect2D(10, 20, 20, 20));
	secondParent.setGeometry(spk::Rect2D(100, 200, 15, 15));
	child.setGeometry(spk::Rect2D(12, 10, 10, 10));

	EXPECT_EQ(child.geometry(), spk::Rect2D(12, 10, 10, 10));
	EXPECT_EQ(child.absoluteGeometryForTest(), spk::Rect2D(22, 30, 10, 10));
	EXPECT_EQ(child.scissorForTest(), spk::Rect2D(22, 30, 8, 10));

	child.setParent(&secondParent);

	EXPECT_EQ(child.geometry(), spk::Rect2D(12, 10, 10, 10));
	EXPECT_EQ(child.absoluteGeometryForTest(), spk::Rect2D(112, 210, 10, 10));
	EXPECT_EQ(child.scissorForTest(), spk::Rect2D(112, 210, 3, 5));
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

TEST(WidgetTest, AllFrameEventTypesReachHandlers)
{
	sparkle_test::RecordingWidget widget("Widget");
	widget.activate();

	const std::vector<spk::FrameEventRecord> events = {
		spk::FrameEventRecord(spk::makeEventRecord(spk::WindowCloseRequestedRecord{})),
		spk::FrameEventRecord(spk::makeEventRecord(spk::WindowDestroyedRecord{})),
		spk::FrameEventRecord(spk::makeEventRecord(spk::WindowMovedRecord{.position = {0, 0}})),
		spk::FrameEventRecord(spk::makeEventRecord(spk::WindowFocusGainedRecord{})),
		spk::FrameEventRecord(spk::makeEventRecord(spk::WindowFocusLostRecord{})),
	};

	for (auto event : events)
	{
		widget.dispatchFrameEvent(event);
	}

	EXPECT_EQ(widget.frameEventCount, static_cast<int>(events.size()));
}

TEST(WidgetTest, AllMouseEventTypesReachHandlers)
{
	sparkle_test::RecordingWidget widget("Widget");
	widget.activate();
	spk::Mouse mouse;

	const std::vector<spk::MouseEventRecord> events = {
		spk::MouseEventRecord(spk::makeEventRecord(spk::MouseEnteredRecord{})),
		spk::MouseEventRecord(spk::makeEventRecord(spk::MouseLeftRecord{})),
		spk::MouseEventRecord(spk::makeEventRecord(spk::MouseWheelScrolledRecord{.delta = {0.0f, 1.0f}})),
		spk::MouseEventRecord(spk::makeEventRecord(spk::MouseButtonReleasedRecord{.button = spk::Mouse::Left})),
		spk::MouseEventRecord(spk::makeEventRecord(spk::MouseButtonDoubleClickedRecord{.button = spk::Mouse::Left})),
	};

	for (auto event : events)
	{
		widget.dispatchMouseEvent(event, mouse);
	}

	EXPECT_EQ(widget.mouseEventCount, static_cast<int>(events.size()));
}

TEST(WidgetTest, AllKeyboardEventTypesReachHandlers)
{
	sparkle_test::RecordingWidget widget("Widget");
	widget.activate();
	spk::Keyboard keyboard;

	const std::vector<spk::KeyboardEventRecord> events = {
		spk::KeyboardEventRecord(spk::makeEventRecord(spk::KeyReleasedRecord{.key = spk::Keyboard::Return})),
		spk::KeyboardEventRecord(spk::makeEventRecord(spk::TextInputRecord{.glyph = U'A'})),
	};

	for (auto event : events)
	{
		widget.dispatchKeyboardEvent(event, keyboard);
	}

	EXPECT_EQ(widget.keyboardEventCount, static_cast<int>(events.size()));
}

TEST(WidgetTest, AppendRenderUnitsSkipsWhenScissorIsEmptyButGeometryIsNot)
{
	sparkle_test::RecordingWidget parent("Parent");
	sparkle_test::RecordingWidget child("Child", &parent);
	spk::RenderSnapshotBuilder builder;
	spk::RenderModule renderModule;

	parent.activate();
	child.activate();

	parent.setGeometry(spk::Rect2D(0, 0, 10, 10));
	child.setGeometry(spk::Rect2D(20, 20, 5, 5));

	EXPECT_TRUE(child.scissorForTest().empty());
	EXPECT_FALSE(child.absoluteGeometryForTest().empty());

	child.appendRenderUnits(builder);
	sparkle_test::TestRenderContext renderContext(std::make_shared<sparkle_test::TestSurfaceState>());
	renderModule.publishSnapshot(std::make_shared<spk::RenderSnapshot>(builder.build()));
	renderModule.render(renderContext);

	EXPECT_EQ(child.appendRenderCommandCount, 0);
}

TEST(WidgetTest, BaseWidgetDefaultEventHandlersAreNoOps)
{
	spk::Widget widget("Base");
	widget.activate();

	spk::Mouse mouse;
	spk::Keyboard keyboard;

	spk::FrameEventRecord frameEvent(spk::makeEventRecord(spk::WindowMovedRecord{.position = {10, 20}}));
	spk::MouseEventRecord mouseEvent(spk::makeEventRecord(spk::MouseEnteredRecord{}));
	spk::KeyboardEventRecord keyboardEvent(spk::makeEventRecord(spk::KeyReleasedRecord{.key = spk::Keyboard::Return}));

	EXPECT_NO_THROW(widget.dispatchFrameEvent(frameEvent));
	EXPECT_NO_THROW(widget.dispatchMouseEvent(mouseEvent, mouse));
	EXPECT_NO_THROW(widget.dispatchKeyboardEvent(keyboardEvent, keyboard));
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
