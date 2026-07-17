#include <gtest/gtest.h>

#include <memory>
#include <string>
#include <string_view>
#include <vector>

#include "structures/game_engine/rendering/spk_scene_render_pipeline.hpp"
#include "structures/game_engine/rendering/spk_scene_render_passes.hpp"
#include "structures/game_engine/rendering/spk_scene_render_priorities.hpp"
#include "structures/game_engine/rendering/spk_shadow_render_pass.hpp"
#include "structures/game_engine/spk_component_logic_registry.hpp"
#include "structures/graphics/rendering/pipeline/spk_render_pipeline.hpp"
#include "structures/graphics/spk_framebuffer_object.hpp"

namespace
{
	inline constexpr std::string_view PostPass = "spk.test.post";

	class MarkerCommand : public spk::RenderCommand
	{
	public:
		int marker;
		explicit MarkerCommand(int p_marker) : marker(p_marker) {}
		void execute(spk::RenderContext &) override {}
	};

	class CasterComponent : public spk::Component {};

	class MultiPassLogic : public spk::ComponentLogic<CasterComponent>
	{
	public:
		int collections = 0;
		std::vector<std::uint32_t> cascades;

	protected:
		void _executeRender(const spk::SceneRenderBuildContext &p_context) override
		{
			++collections;
			auto &opaque = p_context.frame.passes.require(spk::SceneRenderPasses::MainOpaque);
			opaque.emplace<MarkerCommand>(1000);
			for (spk::ShadowRenderPass &shadow : p_context.frame.passes.all<spk::ShadowRenderPass>())
			{
				cascades.push_back(shadow.cascadeIndex());
				shadow.emplace<MarkerCommand>(static_cast<int>(shadow.cascadeIndex()));
			}
		}
	};

	class ShadowAndPostFeature : public spk::ISceneRenderPipelineFeature
	{
	private:
		std::vector<std::unique_ptr<spk::FrameBufferObject>> _targets;

	public:
		explicit ShadowAndPostFeature(std::size_t p_cascades)
		{
			for (std::size_t index = 0; index < p_cascades; ++index)
			{
				_targets.push_back(std::make_unique<spk::FrameBufferObject>(spk::FrameBufferObject::depthTextureTarget({8, 8})));
			}
		}

		void declarePasses(const spk::SceneRenderBuildContext &p_context, spk::RenderPipeline &p_passes) override
		{
			for (std::uint32_t cascade = 0; cascade < _targets.size(); ++cascade)
			{
				const std::string name = "DirectionalShadow[" + std::to_string(cascade) + "]";
				p_passes.emplace<spk::ShadowRenderPass>(
					name,
					spk::SceneRenderPriorities::ShadowBase + static_cast<int>(cascade),
					{.target = {.frameBuffer = _targets[cascade].get(), .viewport = _targets[cascade]->viewport()}, .clear = {.depth = 1.0f}},
					cascade, spk::Matrix4x4::identity());
			}
			p_passes.emplace<spk::RenderPass>(
				std::string(PostPass), spk::SceneRenderPriorities::PostProcessingBase,
				{.target = p_context.request.mainTarget, .clear = {}});
		}
	};

	spk::SceneRenderFrameRequest requestFor(spk::FrameBufferObject &p_target)
	{
		return {.mainTarget = {.frameBuffer = &p_target, .viewport = p_target.viewport()},
			.mainClear = {.color = spk::Color(0, 0, 0, 0), .depth = 1.0f, .stencil = 0}};
	}
}

TEST(SceneRenderPipelineTest, DefaultsAreDirectScopedPhysicalPasses)
{
	spk::FrameBufferObject target(spk::FrameBufferObject::colorTarget({8, 8}));
	spk::RenderPipeline passes;
	spk::RenderFrameBuildContext frame{.passes = passes};
	spk::ComponentLogicRegistry logics;
	spk::ComponentRegistry components;
	spk::SceneRenderPipeline pipeline;
	pipeline.buildPasses(frame, requestFor(target), logics, components);
	ASSERT_EQ(passes.size(), 3u);
	EXPECT_TRUE(passes.require(spk::SceneRenderPasses::MainOpaque).description().clear.color.has_value());
	EXPECT_FALSE(passes.require(spk::SceneRenderPasses::MainTransparent).description().clear.color.has_value());
	EXPECT_FALSE(passes.require(spk::SceneRenderPasses::MainOverlay).description().clear.color.has_value());
	spk::RenderPlan plan = passes.build();
	EXPECT_EQ(plan.diagnostics()[0].id, spk::SceneRenderPasses::MainOpaque);
	EXPECT_EQ(plan.diagnostics()[2].id, spk::SceneRenderPasses::MainOverlay);
}

TEST(SceneRenderPipelineTest, FeaturePriorityAndTypedRepeatedPassesProduceEndToEndOrder)
{
	spk::FrameBufferObject target(spk::FrameBufferObject::colorTarget({8, 8}));
	spk::RenderPipeline passes;
	spk::RenderFrameBuildContext frame{.passes = passes};
	spk::ComponentLogicRegistry logics;
	auto &logic = logics.add<MultiPassLogic>();
	spk::ComponentRegistry components;
	spk::SceneRenderPipeline pipeline;
	pipeline.emplaceFeature<ShadowAndPostFeature>(3);
	pipeline.buildPasses(frame, requestFor(target), logics, components);

	EXPECT_EQ(logic.collections, 1);
	EXPECT_EQ(logic.cascades, (std::vector<std::uint32_t>{0, 1, 2}));
	spk::RenderPlan plan = passes.build();
	const auto diagnostics = plan.diagnostics();
	ASSERT_EQ(diagnostics.size(), 7u);
	EXPECT_EQ(diagnostics[0].id, "DirectionalShadow[0]");
	EXPECT_EQ(diagnostics[1].id, "DirectionalShadow[1]");
	EXPECT_EQ(diagnostics[2].id, "DirectionalShadow[2]");
	EXPECT_EQ(diagnostics[3].id, spk::SceneRenderPasses::MainOpaque);
	EXPECT_EQ(diagnostics[4].id, spk::SceneRenderPasses::MainTransparent);
	EXPECT_EQ(diagnostics[5].id, spk::SceneRenderPasses::MainOverlay);
	EXPECT_EQ(diagnostics[6].id, PostPass);
	EXPECT_TRUE(diagnostics[3].clear.color.has_value());
	EXPECT_FALSE(diagnostics[4].clear.color.has_value());
	EXPECT_FALSE(diagnostics[5].clear.color.has_value());
}

TEST(SceneRenderPipelineTest, MissingRequiredPassAndOptionalAbsentShadowBehaveDifferently)
{
	spk::RenderPipeline passes;
	EXPECT_THROW((void)passes.require(spk::SceneRenderPasses::MainOpaque), std::invalid_argument);
	EXPECT_EQ(passes.find("shadow[0]"), nullptr);
	EXPECT_TRUE(passes.all<spk::ShadowRenderPass>().empty());
}
