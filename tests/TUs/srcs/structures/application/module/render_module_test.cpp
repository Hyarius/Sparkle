#include <gtest/gtest.h>

#include <memory>
#include <utility>

#include "structures/application/module/spk_render_module.hpp"
#include "structures/system/device/window/window_test_utils.hpp"

namespace
{
	void publishAndRender(spk::RenderModule& p_module, spk::RenderSnapshot p_snapshot)
	{
		sparkle_test::TestRenderContext renderContext(std::make_shared<spk::SurfaceState>());

		p_module.publishSnapshot(std::make_shared<spk::RenderSnapshot>(std::move(p_snapshot)));
		p_module.render(renderContext);
	}
}

TEST(RenderModuleTest, RenderExecutesCommandsAndIsSafeWithEmptyBuilder)
{
	spk::RenderModule module;
	spk::RenderSnapshotBuilder builder;

	EXPECT_NO_THROW(publishAndRender(module, builder.build()));

	sparkle_test::RecordingWidget widget("RenderWidget");
	widget.activate();

	widget.appendRenderUnits(builder);
	publishAndRender(module, builder.build());

	EXPECT_EQ(widget.appendRenderCommandCount, 1);
	EXPECT_EQ(widget.renderCount, 1);
}

TEST(RenderModuleTest, RenderUnitBuilderPreservesCommandInsertionOrder)
{
	spk::RenderUnitBuilder builder;
	std::vector<int> calls;

	builder.emplace<sparkle_test::CallbackRenderCommand>([&calls]() { calls.push_back(1); });
	builder.emplace<sparkle_test::CallbackRenderCommand>([&calls]() { calls.push_back(2); });
	builder.emplace<sparkle_test::CallbackRenderCommand>([&calls]() { calls.push_back(3); });

	spk::RenderUnit unit = builder.build();
	sparkle_test::TestRenderContext renderContext(std::make_shared<spk::SurfaceState>());
	unit.execute(renderContext);

	EXPECT_EQ(calls, std::vector<int>({1, 2, 3}));
}

TEST(RenderModuleTest, RenderUnitBuilderReportsSizeAndCanBeCleared)
{
	spk::RenderUnitBuilder builder;

	EXPECT_TRUE(builder.empty());
	EXPECT_EQ(builder.size(), 0);

	builder.emplace<sparkle_test::CallbackRenderCommand>([]() {});
	builder.emplace<sparkle_test::CallbackRenderCommand>([]() {});

	EXPECT_FALSE(builder.empty());
	EXPECT_EQ(builder.size(), 2);

	builder.clear();

	EXPECT_TRUE(builder.empty());
	EXPECT_EQ(builder.size(), 0);
}

TEST(RenderModuleTest, RenderUnitExposesCommandsAndReportsSize)
{
	spk::RenderUnitBuilder builder;

	builder.emplace<sparkle_test::CallbackRenderCommand>([]() {});
	builder.emplace<sparkle_test::CallbackRenderCommand>([]() {});

	spk::RenderUnit unit = builder.build();

	EXPECT_FALSE(unit.empty());
	EXPECT_EQ(unit.size(), 2);
	EXPECT_EQ(unit.commands().size(), 2);
}

TEST(RenderModuleTest, RenderSnapshotBuilderIgnoresEmptyUnitsAndExposesState)
{
	spk::RenderSnapshotBuilder snapshotBuilder;
	spk::RenderUnitBuilder emptyUnitBuilder;
	spk::RenderUnitBuilder commandUnitBuilder;
	commandUnitBuilder.emplace<sparkle_test::CallbackRenderCommand>([]() {});

	EXPECT_TRUE(snapshotBuilder.empty());
	EXPECT_EQ(snapshotBuilder.size(), 0);

	snapshotBuilder.append(nullptr);
	snapshotBuilder.append(std::make_shared<spk::RenderUnit>(emptyUnitBuilder.build()));

	EXPECT_TRUE(snapshotBuilder.empty());

	const auto unit = std::make_shared<spk::RenderUnit>(commandUnitBuilder.build());
	snapshotBuilder.append(unit);

	EXPECT_FALSE(snapshotBuilder.empty());
	EXPECT_EQ(snapshotBuilder.size(), 1);
	ASSERT_EQ(snapshotBuilder.units().size(), 1);
	EXPECT_EQ(snapshotBuilder.units().front(), unit);

	snapshotBuilder.clear();

	EXPECT_TRUE(snapshotBuilder.empty());
	EXPECT_EQ(snapshotBuilder.size(), 0);
}

TEST(RenderModuleTest, RenderSnapshotExposesUnitsAndReportsSize)
{
	spk::RenderUnitBuilder unitBuilder;
	unitBuilder.emplace<sparkle_test::CallbackRenderCommand>([]() {});
	const auto unit = std::make_shared<spk::RenderUnit>(unitBuilder.build());

	spk::RenderSnapshot snapshot(std::vector<std::shared_ptr<spk::RenderUnit>>{unit});

	EXPECT_FALSE(snapshot.empty());
	EXPECT_EQ(snapshot.size(), 1);
	ASSERT_EQ(snapshot.units().size(), 1);
	EXPECT_EQ(snapshot.units().front(), unit);
}

TEST(RenderModuleTest, RenderCommandReceivesRenderContext)
{
	spk::RenderModule module;
	spk::RenderUnitBuilder builder;
	sparkle_test::TestRenderContext renderContext(std::make_shared<spk::SurfaceState>());
	spk::RenderContext* seenContext = nullptr;

	builder.emplace<sparkle_test::CallbackRenderCommand>([&seenContext](spk::RenderContext& p_renderContext) { seenContext = &p_renderContext; });

	module.publishSnapshot(
		std::make_shared<spk::RenderSnapshot>(
			spk::RenderSnapshot(std::vector<std::shared_ptr<spk::RenderUnit>>{std::make_shared<spk::RenderUnit>(builder.build())})));

	module.render(renderContext);

	EXPECT_EQ(seenContext, &renderContext);
}
