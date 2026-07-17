#include <gtest/gtest.h>

#include <memory>
#include <string>
#include <vector>

#include "structures/game_engine/rendering/spk_shadow_render_pass.hpp"
#include "structures/graphics/opengl/spk_opengl_clear_command.hpp"
#include "structures/graphics/rendering/pipeline/spk_render_pipeline.hpp"
#include "structures/graphics/rendering/command/spk_use_framebuffer_render_command.hpp"
#include "structures/graphics/spk_framebuffer_object.hpp"

namespace
{
	class MarkerCommand : public spk::RenderCommand
	{
	public:
		int marker;
		explicit MarkerCommand(int p_marker) : marker(p_marker) {}
		void execute(spk::RenderContext &) override {}
	};

	class OtherDerivedPass : public spk::RenderPass
	{
	public:
		using spk::RenderPass::RenderPass;
	};

	spk::RenderPass::Description description()
	{
		return {.target = {.frameBuffer = nullptr, .viewport = spk::Viewport(spk::Rect2D(0, 0, 16, 16))}, .clear = {}};
	}
}

TEST(RenderPipelineTest, StringIdsProvideDirectLookupAndTypedFiltering)
{
	spk::RenderPipeline pipeline;
	auto &pass = pipeline.emplace<spk::RenderPass>("main", 10, description());
	EXPECT_TRUE(pipeline.contains("main"));
	EXPECT_EQ(&pipeline.require("main"), &pass);
	EXPECT_EQ(pipeline.find("missing"), nullptr);
	EXPECT_THROW((void)pipeline.require("missing"), std::invalid_argument);
	EXPECT_THROW((void)pipeline.emplace<OtherDerivedPass>("main", 20, description()), std::invalid_argument);
	EXPECT_EQ(pipeline.all<OtherDerivedPass>().size(), 0u);
}

TEST(RenderPipelineTest, PassesAreSortedByPriorityAndCommandsStayInInsertionOrder)
{
	spk::RenderPipeline pipeline;
	auto &late = pipeline.emplace<spk::RenderPass>("late", 100, description());
	late.emplace<MarkerCommand>(3);
	auto &first = pipeline.emplace<spk::RenderPass>("first", 0, description());
	first.emplace<MarkerCommand>(1);
	first.emplace<MarkerCommand>(2);

	const auto diagnostics = pipeline.diagnostics(true);
	ASSERT_EQ(diagnostics.size(), 2u);
	EXPECT_EQ(diagnostics[0].id, "first");
	EXPECT_EQ(diagnostics[1].id, "late");
	EXPECT_EQ(diagnostics[0].commandCount, 2u);

	spk::RenderPlan plan = pipeline.build();
	EXPECT_THROW((void)pipeline.build(), std::logic_error);
	EXPECT_THROW(first.emplace<MarkerCommand>(99), std::logic_error);
	ASSERT_EQ(plan.passes().size(), 2u);
	EXPECT_EQ(plan.passes()[0]->id(), "first");

	spk::RenderUnit unit = plan.compile();
	std::vector<int> markers;
	for (const auto &command : unit.commands())
	{
		if (const auto *marker = dynamic_cast<const MarkerCommand *>(command.get()); marker != nullptr)
		{
			markers.push_back(marker->marker);
		}
	}
	EXPECT_EQ(markers, (std::vector<int>{1, 2, 3}));
	EXPECT_THROW((void)plan.compile(), std::logic_error);
}

TEST(RenderPipelineTest, DerivedPassesRemainTypedAndPolymorphic)
{
	spk::FrameBufferObject target(spk::FrameBufferObject::depthTextureTarget({16, 16}));
	spk::RenderPipeline pipeline;
	auto &shadow = pipeline.emplace<spk::ShadowRenderPass>(
		"shadow[2]", 102,
		{.target = {.frameBuffer = &target, .viewport = target.viewport()}, .clear = {.depth = 1.0f}},
		2u,
		spk::Matrix4x4::identity());
	EXPECT_EQ(shadow.cascadeIndex(), 2u);
	EXPECT_EQ(&pipeline.require<spk::ShadowRenderPass>("shadow[2]"), &shadow);
	EXPECT_EQ(pipeline.all<spk::ShadowRenderPass>().size(), 1u);
	spk::RenderPlan plan = pipeline.build();
	ASSERT_NE(dynamic_cast<spk::ShadowRenderPass *>(plan.passes()[0].get()), nullptr);
	EXPECT_NO_THROW((void)plan.compile());
}
