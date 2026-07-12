#include "structures/game_engine/rendering/spk_scene_render_pipeline.hpp"

#include <stdexcept>

#include "structures/game_engine/spk_camera_3d.hpp"
#include "structures/game_engine/spk_component_logic_registry.hpp"
#include "structures/game_engine/rendering/spk_scene_render_passes.hpp"
#include "structures/game_engine/rendering/spk_scene_render_priorities.hpp"
#include "structures/graphics/rendering/command/spk_camera_update_render_command.hpp"
#include "structures/graphics/rendering/command/spk_directional_light_update_render_command.hpp"
#include "structures/graphics/spk_directional_light.hpp"
#include "structures/system/spk_profiler.hpp"

namespace spk
{
	void SceneRenderPipeline::addFeature(std::unique_ptr<spk::ISceneRenderPipelineFeature> p_feature)
	{
		if (p_feature == nullptr)
		{
			throw std::invalid_argument("Cannot register a null scene render pipeline feature");
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

	void SceneRenderPipeline::_appendDefaultFrameState(const spk::SceneRenderBuildContext &p_context) const
	{
		if (p_context.mainCamera == nullptr)
		{
			return;
		}
		for (const auto type : {spk::SceneRenderPasses::MainOpaque, spk::SceneRenderPasses::MainTransparent, spk::SceneRenderPasses::MainOverlay})
		{
			auto &pass = p_context.frame.passes.require({.type = type, .scope = p_context.sceneScope, .instance = 0});
			auto setup = pass.contribute(spk::RenderContributionPriorities::PassSetup, 0);
			setup.emplace<spk::CameraUpdateRenderCommand>(1u, p_context.mainCamera->viewProjectionMatrix());
			setup.emplace<spk::DirectionalLightUpdateRenderCommand>(3u, spk::DirectionalLight{.direction = spk::Vector3(1.0f, -2.0f, 0.5f).normalized(), .color = spk::Color(1.0f, 0.95f, 0.85f), .ambient = 0.35f});
		}
	}

	void SceneRenderPipeline::buildPasses(spk::RenderFrameBuildContext &p_frame, spk::RenderPass::ScopeId p_sceneScope, const spk::SceneRenderFrameRequest &p_request, spk::ComponentLogicRegistry &p_logics, spk::ComponentRegistry &p_components, spk::Profiler *p_profiler, bool p_dispatchRendering)
	{
		spk::SceneRenderBuildContext context{.frame = p_frame, .sceneScope = p_sceneScope, .request = p_request, .components = p_components, .mainCamera = spk::Camera3D::mainCamera(), .profiler = p_profiler};
		p_frame.passes.registerPassType(spk::SceneRenderPasses::MainOpaque, "spk.scene.main_opaque");
		p_frame.passes.registerPassType(spk::SceneRenderPasses::MainTransparent, "spk.scene.main_transparent");
		p_frame.passes.registerPassType(spk::SceneRenderPasses::MainOverlay, "spk.scene.main_overlay");
		p_frame.passes.emplacePass<spk::RenderPass>(
			{.type = spk::SceneRenderPasses::MainOpaque, .scope = p_sceneScope, .instance = 0},
			spk::SceneRenderPriorities::MainOpaque,
			"MainOpaque",
			{.debugName = "MainOpaque", .target = p_request.mainTarget, .clear = p_request.mainClear});
		p_frame.passes.emplacePass<spk::RenderPass>(
			{.type = spk::SceneRenderPasses::MainTransparent, .scope = p_sceneScope, .instance = 0},
			spk::SceneRenderPriorities::MainTransparent,
			"MainTransparent",
			{.debugName = "MainTransparent", .target = p_request.mainTarget, .clear = {}});
		p_frame.passes.emplacePass<spk::RenderPass>(
			{.type = spk::SceneRenderPasses::MainOverlay, .scope = p_sceneScope, .instance = 0},
			spk::SceneRenderPriorities::MainOverlay,
			"MainOverlay",
			{.debugName = "MainOverlay", .target = p_request.mainTarget, .clear = {}});

		if (p_dispatchRendering)
		{
			for (const auto &feature : _features)
			{
				feature->prepareFrame(context);
			}
			for (const auto &feature : _features)
			{
				feature->declarePasses(context, p_frame.passes);
			}
			_appendDefaultFrameState(context);
			for (const auto &feature : _features)
			{
				feature->contribute(context);
			}
			p_logics.render(context, p_components);
		}
	}
}
