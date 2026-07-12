#pragma once

#include <concepts>
#include <memory>
#include <utility>
#include <vector>

#include "structures/game_engine/rendering/spk_scene_render_build_context.hpp"
#include "structures/game_engine/rendering/spk_scene_render_pipeline_feature.hpp"
#include "structures/graphics/rendering/pass/spk_render_pass_bucket_pack.hpp"

namespace spk
{
	class ComponentLogicRegistry;
	class ComponentRegistry;

	class SceneRenderPipeline
	{
	private:
		std::vector<std::unique_ptr<spk::ISceneRenderPipelineFeature>> _features;
		void _appendDefaultFrameState(const spk::SceneRenderBuildContext &p_context) const;

	public:
		void addFeature(std::unique_ptr<spk::ISceneRenderPipelineFeature> p_feature);

		template <typename TFeature, typename... TArguments>
			requires std::derived_from<TFeature, spk::ISceneRenderPipelineFeature>
		TFeature &emplaceFeature(TArguments &&...p_arguments)
		{
			auto feature = std::make_unique<TFeature>(std::forward<TArguments>(p_arguments)...);
			TFeature &result = *feature;
			addFeature(std::move(feature));
			return result;
		}

		template <typename TFeature>
			requires std::derived_from<TFeature, spk::ISceneRenderPipelineFeature>
		[[nodiscard]] TFeature *findFeature() noexcept
		{
			for (const auto &feature : _features)
			{
				if (auto *typed = dynamic_cast<TFeature *>(feature.get()); typed != nullptr)
					return typed;
			}
			return nullptr;
		}

		void clearFeatures() noexcept;
		[[nodiscard]] std::size_t featureCount() const noexcept;
		void buildPasses(
			spk::RenderFrameBuildContext &p_frame,
			spk::RenderPass::ScopeId p_sceneScope,
			const spk::SceneRenderFrameRequest &p_request,
			spk::ComponentLogicRegistry &p_logics,
			spk::ComponentRegistry &p_components,
			spk::Profiler *p_profiler = nullptr,
			bool p_dispatchRendering = true);
	};
}
