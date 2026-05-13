#include <gtest/gtest.h>

#include "spk_render_module.hpp"
#include "window_test_utils.hpp"

TEST(RenderModuleTest, RenderExecutesCommandsAndIsSafeWithEmptyBuilder)
{
	spk::RenderModule module;
	spk::RenderCommandBuilder builder;

	EXPECT_NO_THROW(module.render(builder));

	sparkle_test::RecordingWidget widget("RenderWidget");
	widget.activate();

	widget.appendRenderCommands(builder);
	module.render(builder);

	EXPECT_EQ(widget.appendRenderCommandCount, 1);
	EXPECT_EQ(widget.renderCount, 1);
}
