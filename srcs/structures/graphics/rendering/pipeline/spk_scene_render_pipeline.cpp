#include "structures/graphics/rendering/pipeline/spk_scene_render_pipeline.hpp"

#include <optional>
#include <stdexcept>

#include "structures/game_engine/spk_camera_3d.hpp"
#include "structures/game_engine/spk_component_logic_registry.hpp"
#include "structures/graphics/rendering/command/spk_camera_update_render_command.hpp"
#include "structures/graphics/rendering/command/spk_directional_light_update_render_command.hpp"
#include "structures/graphics/spk_directional_light.hpp"
#include "structures/system/spk_profiler.hpp"

namespace spk
{
	void SceneRenderPipeline::addFeature(std::unique_ptr<spk::IRenderPipelineFeature> p_feature)
	{
		if (p_feature == nullptr)
		{
			throw std::invalid_argument("Cannot register a null render pipeline feature");
		}
		_features.push_back(std::move(p_feature));
	}

	void SceneRenderPipeline::clearFeatures() noexcept
	{
		_features.clear();
	}
	std::size_t SceneRenderPipeline::featureCount() const noexcept
	{
		return _features.size();
	}

	void SceneRenderPipeline::_appendDefaultFrameState(
		RenderPass &p_mainPass,
		const RenderFrameContext &p_context) const
	{
		if (p_context.mainCamera == nullptr)
		{
			return;
		}
		p_mainPass.emplace<spk::CameraUpdateRenderCommand>(
			RenderPhase::PassSetup, 1u, p_context.mainCamera->viewProjectionMatrix());
		p_mainPass.emplace<spk::DirectionalLightUpdateRenderCommand>(
			RenderPhase::PassSetup,
			3u,
			spk::DirectionalLight{
				.direction = spk::Vector3(1.0f, -2.0f, 0.5f).normalized(),
				.color = spk::Color(1.0f, 0.95f, 0.85f),
				.ambient = 0.35f});
	}

	RenderPlan SceneRenderPipeline::buildPlan(
		const RenderFrameRequest &p_request,
		spk::ComponentLogicRegistry &p_logics,
		spk::ComponentRegistry &p_components,
		spk::Profiler *p_profiler,
		std::size_t p_frameIndex,
		bool p_dispatchRendering)
	{
		RenderFrameContext context{
			.request = p_request,
			.components = p_components,
			.mainCamera = spk::Camera3D::mainCamera(),
			.profiler = p_profiler,
			.frameIndex = p_frameIndex};
		RenderPipelineBuilder builder(p_logics, p_components, p_profiler);

		if (p_dispatchRendering)
		{
			for (const auto &feature : _features)
			{
				feature->prepareFrame(context);
			}
			for (const auto &feature : _features)
			{
				feature->appendPreMainPasses(builder, context);
			}
		}

		builder.closePreMainPasses();
		std::optional<spk::Profiler::Probe::Sample> mainSample;
		if (p_profiler != nullptr)
		{
			mainSample.emplace(p_profiler->probe("Render/MainScene"));
		}
		RenderPass &mainPass = builder.addPass(RenderPassDescription{.id = RenderPassId{.kind = RenderPassKind::MainScene, .instance = 0, .debugName = "MainScene"}, .target = p_request.mainTarget, .clear = p_request.mainClear, .phases = {RenderPhase::PassSetup, RenderPhase::SceneOpaque, RenderPhase::SceneTransparent, RenderPhase::SceneOverlay}});

		if (p_dispatchRendering)
		{
			_appendDefaultFrameState(mainPass, context);
			for (const auto &feature : _features)
			{
				feature->appendMainPassSetup(mainPass, context);
			}
		}
		for (RenderPhase phase : mainPass.description().phases)
		{
			const RenderPhaseContext phaseContext{
				.frame = context,
				.pass = mainPass.description(),
				.phase = phase,
				.passInstance = mainPass.description().id.instance};
			if (p_dispatchRendering)
			{
				builder.renderPhase(mainPass, phase, phaseContext);
			}
		}
		mainSample.reset();

		builder.beginPostMainPasses();
		if (p_dispatchRendering)
		{
			for (const auto &feature : _features)
			{
				feature->appendPostMainPasses(builder, context);
			}
		}
		RenderPlan result = builder.build();
		std::size_t mainCount = 0;
		for (const RenderPass &pass : result.passes())
		{
			if (pass.description().id.kind == RenderPassKind::MainScene)
			{
				++mainCount;
			}
		}
		if (mainCount != 1)
		{
			throw std::logic_error("A scene render plan must contain exactly one MainScene pass");
		}
		return result;
	}
}
