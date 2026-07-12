#include <gtest/gtest.h>

#include <memory>
#include <vector>

#include "structures/game_engine/rendering/spk_scene_render_pipeline.hpp"
#include "structures/game_engine/rendering/spk_scene_render_passes.hpp"
#include "structures/game_engine/rendering/spk_scene_render_priorities.hpp"
#include "structures/game_engine/rendering/spk_shadow_render_pass.hpp"
#include "structures/game_engine/spk_component_logic_registry.hpp"
#include "structures/graphics/rendering/pass/spk_render_pass_bucket_pack.hpp"
#include "structures/graphics/spk_framebuffer_object.hpp"

namespace
{
	inline constexpr auto PostPass = spk::makeRenderPassTypeId("spk.test.post");

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
			auto &opaque = p_context.frame.passes.require({.type = spk::SceneRenderPasses::MainOpaque, .scope = p_context.sceneScope});
			opaque.contribute(renderPriority(spk::SceneRenderPasses::MainOpaque), p_context.contributorRegistrationOrder).emplace<MarkerCommand>(1000);
			for (spk::ShadowRenderPass &shadow : p_context.frame.passes.passesOfType<spk::ShadowRenderPass>(
					 spk::LightingRenderPasses::DirectionalShadow, p_context.sceneScope))
			{
				cascades.push_back(shadow.cascadeIndex());
				shadow.contribute(renderPriority(spk::LightingRenderPasses::DirectionalShadow), p_context.contributorRegistrationOrder)
					.emplace<MarkerCommand>(static_cast<int>(shadow.cascadeIndex()));
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

		void declarePasses(const spk::SceneRenderBuildContext &p_context, spk::RenderPassBucketPack &p_passes) override
		{
			for (std::uint32_t cascade = 0; cascade < _targets.size(); ++cascade)
			{
				const std::string name = "DirectionalShadow[" + std::to_string(cascade) + "]";
				p_passes.emplacePass<spk::ShadowRenderPass>(
					{.type = spk::LightingRenderPasses::DirectionalShadow, .scope = p_context.sceneScope, .instance = cascade},
					spk::SceneRenderPriorities::ShadowBase + static_cast<int>(cascade), name,
					{.debugName = name, .target = {.frameBuffer = _targets[cascade].get(), .viewport = _targets[cascade]->viewport()}, .clear = {.depth = 1.0f}},
					cascade, spk::Matrix4x4::identity());
			}
			p_passes.emplacePass<spk::RenderPass>(
				{.type = PostPass, .scope = p_context.sceneScope}, spk::SceneRenderPriorities::PostProcessingBase,
				"Post", {.debugName = "Post", .target = p_context.request.mainTarget, .clear = {}});
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
	spk::RenderPassBucketPack passes;
	spk::RenderFrameBuildContext frame{.passes = passes};
	spk::ComponentLogicRegistry logics;
	spk::ComponentRegistry components;
	spk::SceneRenderPipeline pipeline;
	const spk::RenderPass::ScopeId scope{9};
	pipeline.buildPasses(frame, scope, requestFor(target), logics, components);
	ASSERT_EQ(passes.size(), 3u);
	EXPECT_TRUE(passes.require({.type = spk::SceneRenderPasses::MainOpaque, .scope = scope}).description().clear.color.has_value());
	EXPECT_FALSE(passes.require({.type = spk::SceneRenderPasses::MainTransparent, .scope = scope}).description().clear.color.has_value());
	EXPECT_FALSE(passes.require({.type = spk::SceneRenderPasses::MainOverlay, .scope = scope}).description().clear.color.has_value());
	spk::RenderPlan plan = passes.build();
	EXPECT_EQ(plan.diagnostics()[0].debugName, "MainOpaque");
	EXPECT_EQ(plan.diagnostics()[2].debugName, "MainOverlay");
}

TEST(SceneRenderPipelineTest, FeaturePriorityAndTypedRepeatedPassesProduceEndToEndOrder)
{
	spk::FrameBufferObject target(spk::FrameBufferObject::colorTarget({8, 8}));
	spk::RenderPassBucketPack passes;
	spk::RenderFrameBuildContext frame{.passes = passes};
	spk::ComponentLogicRegistry logics;
	auto &logic = logics.add<MultiPassLogic>();
	spk::ComponentRegistry components;
	spk::SceneRenderPipeline pipeline;
	pipeline.emplaceFeature<ShadowAndPostFeature>(3);
	const spk::RenderPass::ScopeId scope{11};
	pipeline.buildPasses(frame, scope, requestFor(target), logics, components);

	EXPECT_EQ(logic.collections, 1);
	EXPECT_EQ(logic.cascades, (std::vector<std::uint32_t>{0, 1, 2}));
	spk::RenderPlan plan = passes.build();
	const auto diagnostics = plan.diagnostics();
	ASSERT_EQ(diagnostics.size(), 7u);
	EXPECT_EQ(diagnostics[0].debugName, "DirectionalShadow[0]");
	EXPECT_EQ(diagnostics[1].debugName, "DirectionalShadow[1]");
	EXPECT_EQ(diagnostics[2].debugName, "DirectionalShadow[2]");
	EXPECT_EQ(diagnostics[3].debugName, "MainOpaque");
	EXPECT_EQ(diagnostics[4].debugName, "MainTransparent");
	EXPECT_EQ(diagnostics[5].debugName, "MainOverlay");
	EXPECT_EQ(diagnostics[6].debugName, "Post");
	EXPECT_TRUE(diagnostics[3].clear.color.has_value());
	EXPECT_FALSE(diagnostics[4].clear.color.has_value());
	EXPECT_FALSE(diagnostics[5].clear.color.has_value());
}

TEST(SceneRenderPipelineTest, MissingRequiredPassAndOptionalAbsentShadowBehaveDifferently)
{
	spk::RenderPassBucketPack passes;
	EXPECT_THROW((void)passes.require({.type = spk::SceneRenderPasses::MainOpaque, .scope = {1}}), std::invalid_argument);
	EXPECT_EQ(passes.find({.type = spk::LightingRenderPasses::DirectionalShadow, .scope = {1}}), nullptr);
	EXPECT_TRUE(passes.passesOfType<spk::ShadowRenderPass>(spk::LightingRenderPasses::DirectionalShadow, spk::RenderPass::ScopeId{1}).empty());
}
