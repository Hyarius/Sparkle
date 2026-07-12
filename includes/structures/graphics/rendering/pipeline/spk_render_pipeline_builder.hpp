#pragma once

#include <cstddef>

#include "structures/graphics/rendering/pipeline/spk_render_plan.hpp"

namespace spk
{
	class ComponentLogicRegistry;
	class ComponentRegistry;

	class RenderPipelineBuilder
	{
	public:
		enum class Stage
		{
			PreMain,
			Main,
			PostMain,
			Built
		};

	private:
		RenderPlan _plan;
		Stage _stage = Stage::PreMain;
		spk::ComponentLogicRegistry *_logics = nullptr;
		spk::ComponentRegistry *_components = nullptr;
		spk::Profiler *_profiler = nullptr;

	public:
		RenderPipelineBuilder() = default;
		RenderPipelineBuilder(
			spk::ComponentLogicRegistry &p_logics,
			spk::ComponentRegistry &p_components,
			spk::Profiler *p_profiler = nullptr) :
			_logics(&p_logics),
			_components(&p_components),
			_profiler(p_profiler)
		{
		}

		RenderPass &addPass(RenderPassDescription p_description);
		void closePreMainPasses();
		void beginPostMainPasses();
		[[nodiscard]] Stage stage() const noexcept;

		void renderPhase(
			RenderPass &p_pass,
			RenderPhase p_phase,
			const RenderPhaseContext &p_context,
			spk::ComponentLogicRegistry &p_logics,
			spk::ComponentRegistry &p_components);
		void renderPhase(RenderPass &p_pass, RenderPhase p_phase, const RenderPhaseContext &p_context);

		[[nodiscard]] RenderPlan build();
	};
}
