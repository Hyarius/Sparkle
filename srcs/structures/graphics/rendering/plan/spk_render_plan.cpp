#include "structures/graphics/rendering/plan/spk_render_plan.hpp"

#include <array>
#include <stdexcept>
#include <typeinfo>
#include <utility>

#include <GL/glew.h>

#include "structures/graphics/opengl/spk_opengl_clear_command.hpp"
#include "structures/graphics/rendering/command/spk_use_framebuffer_render_command.hpp"
#include "structures/graphics/rendering/unit/spk_render_unit_builder.hpp"

namespace spk
{
	spk::RenderPassDiagnostics makeRenderPassDiagnostics(const spk::RenderPass &p_pass)
	{
		return {
			.key = p_pass.key(),
			.debugName = p_pass.debugName(),
			.concreteType = typeid(p_pass).name(),
			.priority = p_pass.priority(),
			.declarationOrder = p_pass.declarationOrder(),
			.defaultTarget = p_pass.description().target.frameBuffer == nullptr,
			.activeTarget = p_pass.description().target.activeTarget,
			.viewport = p_pass.description().target.viewport,
			.clear = p_pass.description().clear,
			.contributorCount = p_pass.contributorCount(),
			.commandCount = p_pass.commandCount()};
	}

	RenderPlan::RenderPlan(std::vector<std::unique_ptr<spk::RenderPass>> p_passes) :
		_passes(std::move(p_passes))
	{
	}

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

	const std::vector<std::unique_ptr<spk::RenderPass>> &RenderPlan::passes() const noexcept
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

	std::vector<spk::RenderPassDiagnostics> RenderPlan::diagnostics() const
	{
		std::vector<spk::RenderPassDiagnostics> result;
		result.reserve(_passes.size());
		for (const auto &pass : _passes)
		{
			result.push_back(spk::makeRenderPassDiagnostics(*pass));
		}
		return result;
	}

	spk::RenderUnit RenderPlan::compile()
	{
		if (_compiled)
		{
			throw std::logic_error("RenderPlan may only be compiled once");
		}
		for (const auto &pass : _passes)
		{
			validateRenderPassDescription(pass->description());
		}
		_compiled = true;

		spk::RenderUnitBuilder builder;
		for (auto &pass : _passes)
		{
			const spk::RenderPass::Description &description = pass->description();
			if (!description.target.activeTarget)
			{
				builder.emplace<spk::UseFrameBufferRenderCommand>(description.target.frameBuffer, description.target.viewport);
			}

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
				builder.emplace<spk::ClearCommand>(color, mask, description.clear.depth.value_or(1.0f), description.clear.stencil.value_or(0));
			}

			for (std::unique_ptr<spk::RenderCommand> &command : pass->takeCommands())
			{
				builder.add(std::move(command));
			}
		}
		return builder.build();
	}
}
