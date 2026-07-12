#include <gtest/gtest.h>

#include <algorithm>
#include <memory>
#include <stdexcept>
#include <vector>

#include "structures/game_engine/spk_component_logic_registry.hpp"
#include "structures/graphics/rendering/pipeline/spk_scene_render_pipeline.hpp"
#include "structures/graphics/spk_framebuffer_object.hpp"
#include "structures/system/spk_profiler.hpp"

namespace
{
	class MarkerCommand : public spk::RenderCommand
	{
	public:
		int marker;
		explicit MarkerCommand(int p_marker) :
			marker(p_marker)
		{
		}
		void execute(spk::RenderContext &) override
		{
		}
	};

	class CasterComponent : public spk::Component
	{
	};
	class ShadowProbeData : public spk::RenderPhaseFeatureData
	{
	public:
		std::size_t cascade;
		spk::Matrix4x4 lightViewProjection = spk::Matrix4x4::identity();
		explicit ShadowProbeData(std::size_t p_cascade) :
			cascade(p_cascade)
		{
		}
	};

	class CasterLogic : public spk::ComponentLogic<CasterComponent>
	{
	public:
		int calls = 0;
		std::vector<std::size_t> cascades;
		[[nodiscard]] spk::RenderPhaseMask renderPhases() const noexcept override
		{
			return spk::renderPhaseBit(spk::RenderPhase::ShadowCaster);
		}

	protected:
		void _executeRender(const spk::RenderPhaseContext &p_context, spk::RenderPass &p_pass) override
		{
			++calls;
			if (const auto *data = p_context.feature<ShadowProbeData>(); data != nullptr)
			{
				cascades.push_back(data->cascade);
			}
			p_pass.emplace<MarkerCommand>(p_context.phase, static_cast<int>(p_context.passInstance));
		}
	};

	class RepeatedShadowFeature : public spk::IRenderPipelineFeature
	{
	private:
		std::vector<std::unique_ptr<spk::FrameBufferObject>> _targets;

	public:
		RepeatedShadowFeature()
		{
			for (int i = 0; i < 3; ++i)
			{
				_targets.push_back(std::make_unique<spk::FrameBufferObject>(spk::FrameBufferObject::depthTextureTarget({8, 8})));
			}
		}

		void appendPreMainPasses(
			spk::RenderPipelineBuilder &p_builder,
			const spk::RenderFrameContext &p_context) override
		{
			for (std::size_t cascade = 0; cascade < _targets.size(); ++cascade)
			{
				spk::RenderPass &pass = p_builder.addPass(spk::RenderPassDescription{.id = {.kind = spk::RenderPassKind::Shadow, .instance = cascade, .debugName = "DirectionalShadow[" + std::to_string(cascade) + "]"}, .target = {.frameBuffer = _targets[cascade].get(), .viewport = _targets[cascade]->viewport()}, .clear = {.depth = 1.0f}, .phases = {spk::RenderPhase::PassSetup, spk::RenderPhase::ShadowCaster}});
				pass.emplace<MarkerCommand>(spk::RenderPhase::PassSetup, 100 + static_cast<int>(cascade));
				const spk::RenderPhaseContext phaseContext{
					.frame = p_context,
					.pass = pass.description(),
					.phase = spk::RenderPhase::ShadowCaster,
					.passInstance = cascade,
					.featureData = std::make_shared<ShadowProbeData>(cascade)};
				p_builder.renderPhase(pass, spk::RenderPhase::ShadowCaster, phaseContext);
			}
		}
	};

	class TraceFeature : public spk::IRenderPipelineFeature
	{
	private:
		std::vector<int> &_trace;
		int _id;

	public:
		TraceFeature(std::vector<int> &p_trace, int p_id) :
			_trace(p_trace),
			_id(p_id)
		{
		}
		void prepareFrame(const spk::RenderFrameContext &) override
		{
			_trace.push_back(_id * 10 + 1);
		}
		void appendPreMainPasses(spk::RenderPipelineBuilder &, const spk::RenderFrameContext &) override
		{
			_trace.push_back(_id * 10 + 2);
		}
		void appendMainPassSetup(spk::RenderPass &p_pass, const spk::RenderFrameContext &) override
		{
			_trace.push_back(_id * 10 + 3);
			p_pass.emplace<MarkerCommand>(spk::RenderPhase::PassSetup, _id);
		}
		void appendPostMainPasses(spk::RenderPipelineBuilder &, const spk::RenderFrameContext &) override
		{
			_trace.push_back(_id * 10 + 4);
		}
	};

	class SurroundingPassFeature : public spk::IRenderPipelineFeature
	{
	public:
		void appendPreMainPasses(spk::RenderPipelineBuilder &p_builder, const spk::RenderFrameContext &p_context) override
		{
			p_builder.addPass(spk::RenderPassDescription{.id = {.kind = spk::RenderPassKind::Auxiliary, .instance = 1, .debugName = "Pre"}, .target = p_context.request.mainTarget, .clear = {}, .phases = {spk::RenderPhase::SceneOverlay}});
		}
		void appendPostMainPasses(spk::RenderPipelineBuilder &p_builder, const spk::RenderFrameContext &p_context) override
		{
			p_builder.addPass(spk::RenderPassDescription{.id = {.kind = spk::RenderPassKind::Auxiliary, .instance = 2, .debugName = "Post"}, .target = p_context.request.mainTarget, .clear = {}, .phases = {spk::RenderPhase::SceneOverlay}});
		}
	};

	class LatePreFeature : public spk::IRenderPipelineFeature
	{
	public:
		void appendPostMainPasses(spk::RenderPipelineBuilder &p_builder, const spk::RenderFrameContext &p_context) override
		{
			p_builder.addPass(spk::RenderPassDescription{.id = {.kind = spk::RenderPassKind::Shadow, .instance = 9, .debugName = "TooLate"}, .target = p_context.request.mainTarget, .clear = {}, .phases = {spk::RenderPhase::ShadowCaster}});
		}
	};

	spk::RenderFrameRequest requestFor(spk::FrameBufferObject &p_target)
	{
		return spk::RenderFrameRequest{
			.mainTarget = {.frameBuffer = &p_target, .viewport = p_target.viewport()},
			.mainClear = {.color = spk::Color(0, 0, 0, 0), .depth = 1.0f, .stencil = 0}};
	}
}

TEST(SceneRenderPipelineTest, NoFeaturesProducesExactlyOneValidMainPass)
{
	spk::FrameBufferObject target(spk::FrameBufferObject::colorTarget({8, 8}));
	spk::ComponentLogicRegistry logics;
	spk::ComponentRegistry components;
	spk::SceneRenderPipeline pipeline;
	spk::RenderPlan plan = pipeline.buildPlan(requestFor(target), logics, components);
	ASSERT_EQ(plan.size(), 1u);
	EXPECT_EQ(plan.passes()[0].description().id.kind, spk::RenderPassKind::MainScene);
}

TEST(SceneRenderPipelineTest, PublishesSemanticMainPassAndPhaseProfilerNames)
{
	spk::FrameBufferObject target(spk::FrameBufferObject::colorTarget({8, 8}));
	spk::ComponentLogicRegistry logics;
	spk::ComponentRegistry components;
	spk::SceneRenderPipeline pipeline;
	spk::Profiler profiler;
	(void)pipeline.buildPlan(requestFor(target), logics, components, &profiler);
	const auto snapshots = profiler.snapshot();
	const auto contains = [&](const std::string &p_name) {
		return std::ranges::any_of(snapshots, [&](const auto &p_snapshot) {
			return p_snapshot.name == p_name;
		});
	};
	EXPECT_TRUE(contains("Render/MainScene"));
	EXPECT_TRUE(contains("Render/MainScene/PassSetup"));
	EXPECT_TRUE(contains("Render/MainScene/SceneOpaque"));
	EXPECT_TRUE(contains("Render/MainScene/SceneTransparent"));
	EXPECT_TRUE(contains("Render/MainScene/SceneOverlay"));
}

TEST(SceneRenderPipelineTest, FeaturesRunInStableRegistrationOrderAndMainSetupIsInPassSetup)
{
	spk::FrameBufferObject target(spk::FrameBufferObject::colorTarget({8, 8}));
	spk::ComponentLogicRegistry logics;
	spk::ComponentRegistry components;
	spk::SceneRenderPipeline pipeline;
	std::vector<int> trace;
	pipeline.emplaceFeature<TraceFeature>(trace, 1);
	pipeline.emplaceFeature<TraceFeature>(trace, 2);
	spk::RenderPlan plan = pipeline.buildPlan(requestFor(target), logics, components);
	EXPECT_EQ(trace, (std::vector<int>{11, 21, 12, 22, 13, 23, 14, 24}));
	EXPECT_EQ(plan.passes()[0].commandCount(spk::RenderPhase::PassSetup), 2u);
}

TEST(SceneRenderPipelineTest, PreAndPostPassesSurroundMain)
{
	spk::FrameBufferObject target(spk::FrameBufferObject::colorTarget({8, 8}));
	spk::ComponentLogicRegistry logics;
	spk::ComponentRegistry components;
	spk::SceneRenderPipeline pipeline;
	pipeline.emplaceFeature<SurroundingPassFeature>();
	spk::RenderPlan plan = pipeline.buildPlan(requestFor(target), logics, components);
	ASSERT_EQ(plan.size(), 3u);
	EXPECT_EQ(plan.passes()[0].description().id.debugName, "Pre");
	EXPECT_EQ(plan.passes()[1].description().id.kind, spk::RenderPassKind::MainScene);
	EXPECT_EQ(plan.passes()[2].description().id.debugName, "Post");
}

TEST(SceneRenderPipelineTest, ThreeRepeatedShadowPassesInvokeSameCasterBeforeMain)
{
	spk::FrameBufferObject target(spk::FrameBufferObject::colorTarget({8, 8}));
	spk::ComponentLogicRegistry logics;
	auto &caster = logics.add<CasterLogic>();
	spk::ComponentRegistry components;
	spk::SceneRenderPipeline pipeline;
	pipeline.emplaceFeature<RepeatedShadowFeature>();
	spk::RenderPlan plan = pipeline.buildPlan(requestFor(target), logics, components);
	ASSERT_EQ(plan.size(), 4u);
	for (std::size_t index = 0; index < 3; ++index)
	{
		EXPECT_EQ(plan.passes()[index].description().id.kind, spk::RenderPassKind::Shadow);
		EXPECT_EQ(plan.passes()[index].description().id.instance, index);
		EXPECT_EQ(plan.passes()[index].commandCount(spk::RenderPhase::ShadowCaster), 1u);
		EXPECT_EQ(plan.passes()[index].contributorCount(spk::RenderPhase::ShadowCaster), 1u);
	}
	EXPECT_EQ(plan.passes()[3].description().id.kind, spk::RenderPassKind::MainScene);
	EXPECT_EQ(caster.calls, 3);
	EXPECT_EQ(caster.cascades, (std::vector<std::size_t>{0, 1, 2}));
}

TEST(SceneRenderPipelineTest, PreMainPassCannotBeAppendedAfterMainStage)
{
	spk::FrameBufferObject target(spk::FrameBufferObject::colorTarget({8, 8}));
	spk::ComponentLogicRegistry logics;
	spk::ComponentRegistry components;
	spk::SceneRenderPipeline pipeline;
	pipeline.emplaceFeature<LatePreFeature>();
	EXPECT_THROW((void)pipeline.buildPlan(requestFor(target), logics, components), std::logic_error);
}
