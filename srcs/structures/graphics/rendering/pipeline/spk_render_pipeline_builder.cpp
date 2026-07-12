#include "structures/graphics/rendering/pipeline/spk_render_pipeline_builder.hpp"

#include <optional>
#include <stdexcept>

#include "structures/game_engine/spk_component_logic_registry.hpp"
#include "structures/system/spk_profiler.hpp"

namespace spk
{
	RenderPass &RenderPipelineBuilder::addPass(RenderPassDescription p_description)
	{
		std::optional<spk::Profiler::Probe::Sample> sample;
		if (_profiler != nullptr)
		{
			sample.emplace(_profiler->probe("Render/" + renderPassDisplayName(p_description.id)));
		}
		if (_stage == Stage::Built)
		{
			throw std::logic_error("Cannot add a pass after RenderPipelineBuilder::build()");
		}
		if (_stage != Stage::PreMain && p_description.id.kind == RenderPassKind::Shadow)
		{
			throw std::logic_error("Cannot append a shadow pre-main pass after main-pass construction has started");
		}
		if (_stage == Stage::PreMain && p_description.id.kind == RenderPassKind::MainScene)
		{
			throw std::logic_error("MainScene may only be added after pre-main passes are closed");
		}
		_plan.addPass(RenderPass(std::move(p_description)));
		return _plan._passes.back();
	}

	void RenderPipelineBuilder::closePreMainPasses()
	{
		if (_stage != Stage::PreMain)
		{
			throw std::logic_error("Pre-main passes are already closed");
		}
		_stage = Stage::Main;
	}

	void RenderPipelineBuilder::beginPostMainPasses()
	{
		if (_stage != Stage::Main)
		{
			throw std::logic_error("Post-main stage requires main-pass construction");
		}
		_stage = Stage::PostMain;
	}

	RenderPipelineBuilder::Stage RenderPipelineBuilder::stage() const noexcept
	{
		return _stage;
	}

	void RenderPipelineBuilder::renderPhase(
		RenderPass &p_pass,
		RenderPhase p_phase,
		const RenderPhaseContext &p_context,
		spk::ComponentLogicRegistry &p_logics,
		spk::ComponentRegistry &p_components)
	{
		std::optional<spk::Profiler::Probe::Sample> sample;
		if (_profiler != nullptr)
		{
			sample.emplace(_profiler->probe(
				"Render/" + renderPassDisplayName(p_pass.description().id) + "/" +
				std::string(renderPhaseName(p_phase))));
		}
		if (&p_context.pass != &p_pass.description() || p_context.phase != p_phase)
		{
			throw std::invalid_argument("RenderPhaseContext does not match requested pass/phase");
		}
		if (!p_pass.declares(p_phase))
		{
			throw std::invalid_argument("Requested phase is not declared by the render pass");
		}
		p_logics.renderPhase(p_context, p_pass, p_components);
	}

	void RenderPipelineBuilder::renderPhase(
		RenderPass &p_pass,
		RenderPhase p_phase,
		const RenderPhaseContext &p_context)
	{
		if (_logics == nullptr || _components == nullptr)
		{
			throw std::logic_error("RenderPipelineBuilder has no contributor registries");
		}
		renderPhase(p_pass, p_phase, p_context, *_logics, *_components);
	}

	RenderPlan RenderPipelineBuilder::build()
	{
		if (_stage == Stage::Built)
		{
			throw std::logic_error("RenderPipelineBuilder may only build once");
		}
		_stage = Stage::Built;
		return std::move(_plan);
	}
}
