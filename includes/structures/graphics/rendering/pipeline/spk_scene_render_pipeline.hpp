#pragma once

#include <concepts>
#include <memory>
#include <utility>
#include <vector>

#include "structures/graphics/rendering/pipeline/spk_render_pipeline_builder.hpp"
#include "structures/graphics/rendering/pipeline/spk_render_pipeline_feature.hpp"

namespace spk
{
	class ComponentLogicRegistry;
	class ComponentRegistry;

	// Owns structural rendering order: registered features append pre-main passes
	// in registration order, then one main pass is built, followed by post-main
	// feature passes in registration order.
	class SceneRenderPipeline
	{
	private:
		std::vector<std::unique_ptr<spk::IRenderPipelineFeature>> _features;

		void _appendDefaultFrameState(RenderPass &p_mainPass, const RenderFrameContext &p_context) const;

	public:
		void addFeature(std::unique_ptr<spk::IRenderPipelineFeature> p_feature);

		template <typename TFeature, typename... TArguments>
			requires std::derived_from<TFeature, spk::IRenderPipelineFeature>
		TFeature &emplaceFeature(TArguments &&...p_arguments)
		{
			auto feature = std::make_unique<TFeature>(std::forward<TArguments>(p_arguments)...);
			TFeature &result = *feature;
			addFeature(std::move(feature));
			return result;
		}

		void clearFeatures() noexcept;
		[[nodiscard]] std::size_t featureCount() const noexcept;

		[[nodiscard]] RenderPlan buildPlan(
			const RenderFrameRequest &p_request,
			spk::ComponentLogicRegistry &p_logics,
			spk::ComponentRegistry &p_components,
			spk::Profiler *p_profiler = nullptr,
			std::size_t p_frameIndex = 0,
			bool p_dispatchRendering = true);
	};
}
