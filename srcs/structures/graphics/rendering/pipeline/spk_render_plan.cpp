#include "structures/graphics/rendering/pipeline/spk_render_plan.hpp"

#include <array>
#include <stdexcept>
#include <utility>

#include <GL/glew.h>

#include "structures/graphics/opengl/spk_opengl_clear_command.hpp"
#include "structures/graphics/rendering/command/spk_use_framebuffer_render_command.hpp"
#include "structures/graphics/rendering/unit/spk_render_unit_builder.hpp"

namespace spk
{
	RenderPlan::RenderPlan(RenderPlan &&p_other) noexcept :
		_passes(std::move(p_other._passes)),
		_compiled(p_other._compiled)
	{
		p_other._compiled = true;
	}

	RenderPlan &RenderPlan::operator=(RenderPlan &&p_other) noexcept
	{
		if (this != &p_other)
		{
			_passes = std::move(p_other._passes);
			_compiled = p_other._compiled;
			p_other._compiled = true;
		}
		return *this;
	}

	void RenderPlan::addPass(RenderPass p_pass)
	{
		if (_compiled)
		{
			throw std::logic_error("Cannot add a pass to an already compiled RenderPlan");
		}
		for (const RenderPass &existing : _passes)
		{
			if (existing.description().id == p_pass.description().id)
			{
				throw std::invalid_argument("Duplicate render pass identity '" + renderPassDisplayName(p_pass.description().id) + "'");
			}
		}
		_passes.push_back(std::move(p_pass));
	}

	const std::vector<RenderPass> &RenderPlan::passes() const noexcept
	{
		return _passes;
	}
	bool RenderPlan::empty() const noexcept
	{
		return _passes.empty();
	}
	std::size_t RenderPlan::size() const noexcept
	{
		return _passes.size();
	}

	std::vector<RenderPassDiagnostics> RenderPlan::diagnostics() const
	{
		std::vector<RenderPassDiagnostics> result;
		result.reserve(_passes.size());
		for (const RenderPass &pass : _passes)
		{
			RenderPassDiagnostics entry{
				.id = pass.description().id,
				.defaultTarget = pass.description().target.frameBuffer == nullptr,
				.clear = pass.description().clear};
			for (RenderPhase phase : pass.description().phases)
			{
				entry.phases.push_back(RenderPhaseDiagnostics{.phase = phase, .contributorCount = pass.contributorCount(phase), .commandCount = pass.commandCount(phase)});
			}
			result.push_back(std::move(entry));
		}
		return result;
	}

	spk::RenderUnit RenderPlan::compile()
	{
		if (_compiled)
		{
			throw std::logic_error("RenderPlan may only be compiled once");
		}
		// Validate the complete snapshot before moving any command ownership.
		for (const RenderPass &pass : _passes)
		{
			validateRenderPassDescription(pass.description());
		}
		_compiled = true;

		spk::RenderUnitBuilder builder;
		for (RenderPass &pass : _passes)
		{
			const RenderPassDescription &description = pass.description();
			builder.emplace<spk::UseFrameBufferRenderCommand>(description.target.frameBuffer, description.target.viewport);

			GLbitfield mask = 0;
			std::array<float, 4> color{0.0f, 0.0f, 0.0f, 0.0f};
			if (description.clear.color.has_value())
			{
				mask |= GL_COLOR_BUFFER_BIT;
				color = description.clear.color->values();
			}
			if (description.clear.depth.has_value())
			{
				mask |= GL_DEPTH_BUFFER_BIT;
			}
			if (description.clear.stencil.has_value())
			{
				mask |= GL_STENCIL_BUFFER_BIT;
			}
			if (mask != 0)
			{
				builder.emplace<spk::ClearCommand>(
					color,
					mask,
					description.clear.depth.value_or(1.0f),
					description.clear.stencil.value_or(0));
			}

			for (RenderPass::PhaseCommands &phase : pass._phaseCommands)
			{
				for (std::unique_ptr<spk::RenderCommand> &command : phase.commands)
				{
					builder.add(std::move(command));
				}
				phase.commands.clear();
			}
		}
		return builder.build();
	}
}
