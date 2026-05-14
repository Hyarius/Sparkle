#include <gtest/gtest.h>

#include <memory>
#include <utility>

#include "spk_render_module.hpp"
#include "window_test_utils.hpp"

namespace
{
	void publishAndRender(spk::RenderModule& p_module, spk::RenderSnapshot p_snapshot)
	{
		p_module.publishSnapshot(std::make_shared<const spk::RenderSnapshot>(std::move(p_snapshot)));
		p_module.render();
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
	unit.execute();

	EXPECT_EQ(calls, std::vector<int>({1, 2, 3}));
}
