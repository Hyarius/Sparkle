#include "structures/graphics/rendering/pass/spk_render_pass.hpp"

#include <algorithm>
#include <cmath>
#include <stdexcept>
#include <utility>

#include "structures/graphics/spk_framebuffer_object.hpp"
#include "structures/graphics/spk_texture.hpp"

namespace spk
{
	void validateRenderPassDescription(const spk::RenderPass::Description &p_description)
	{
		const std::string &name = p_description.debugName;
		if (name.empty())
		{
			throw std::invalid_argument("Render pass debug name cannot be empty");
		}
		if (p_description.target.viewport.geometry().empty() || p_description.target.viewport.scissor().empty())
		{
			throw std::invalid_argument("Render pass '" + name + "': viewport and scissor must be non-empty");
		}
		if (p_description.clear.depth.has_value() &&
			(!std::isfinite(*p_description.clear.depth) || *p_description.clear.depth < 0.0f || *p_description.clear.depth > 1.0f))
		{
			throw std::invalid_argument("Render pass '" + name + "': depth clear must be finite and normalized");
		}

		const spk::FrameBufferObject *target = p_description.target.frameBuffer;
		if (target == nullptr)
		{
			return;
		}
		if (target->size().x == 0 || target->size().y == 0 ||
			!target->surfaceState().rect().contains(p_description.target.viewport.geometry()) ||
			!target->surfaceState().rect().contains(p_description.target.viewport.scissor()))
		{
			throw std::invalid_argument("Render pass '" + name + "': viewport/scissor exceeds the render target");
		}
		if (p_description.clear.color.has_value() && !target->hasColorAttachment())
		{
			throw std::invalid_argument("Render pass '" + name + "': color clear requires a color attachment");
		}
		if (p_description.clear.depth.has_value() && !target->hasDepthAttachment())
		{
			throw std::invalid_argument("Render pass '" + name + "': depth clear requires a depth attachment");
		}
		const auto &depth = target->description().depth;
		const bool hasStencil = depth.has_value() && spk::Texture::isDepthStencilFormat(depth->format);
		if (p_description.clear.stencil.has_value() && !hasStencil)
		{
			throw std::invalid_argument("Render pass '" + name + "': stencil clear requires a stencil attachment");
		}
	}

	RenderPass::RenderPass(
		spk::RenderPass::Key p_key,
		std::int32_t p_priority,
		std::size_t p_declarationOrder,
		std::string p_debugName,
		spk::RenderPass::Description p_description) :
		_key(p_key),
		_debugName(std::move(p_debugName)),
		_description(std::move(p_description)),
		_priority(p_priority),
		_declarationOrder(p_declarationOrder)
	{
		if (_debugName.empty())
		{
			throw std::invalid_argument("Render pass debug name cannot be empty");
		}
		_description.debugName = _debugName;
		validateRenderPassDescription(_description);
	}

	const spk::RenderPass::Key &RenderPass::key() const noexcept
	{
		return _key;
	}

	const std::string &RenderPass::debugName() const noexcept
	{
		return _debugName;
	}

	const spk::RenderPass::Description &RenderPass::description() const noexcept
	{
		return _description;
	}

	std::int32_t RenderPass::priority() const noexcept
	{
		return _priority;
	}

	void RenderPass::setPriority(std::int32_t p_priority)
	{
		(void)p_priority;
		if (_sealed)
		{
			throw std::logic_error("Cannot change priority of sealed render pass '" + _debugName + "'");
		}
		throw std::logic_error("Render pass priority is fixed when '" + _debugName + "' is declared");
	}

	std::size_t RenderPass::declarationOrder() const noexcept
	{
		return _declarationOrder;
	}

	std::size_t RenderPass::contributorCount() const noexcept
	{
		return _contributions.size();
	}

	std::size_t RenderPass::commandCount() const noexcept
	{
		std::size_t result = 0;
		for (const auto &contribution : _contributions)
		{
			result += contribution.commands.size();
		}
		return result;
	}

	spk::RenderContributionSink RenderPass::contribute(
		std::int32_t p_contributorPriority,
		std::size_t p_registrationOrder)
	{
		if (_sealed)
		{
			throw std::logic_error("Cannot contribute to sealed render pass '" + _debugName + "'");
		}
		const std::size_t index = _contributions.size();
		_contributions.push_back(spk::RenderContribution{
			.priority = p_contributorPriority,
			.registrationOrder = p_registrationOrder,
			.declarationOrder = index});
		return spk::RenderContributionSink(*this, index);
	}

	void RenderPass::_add(std::size_t p_contribution, std::unique_ptr<spk::RenderCommand> p_command)
	{
		if (_sealed || p_contribution >= _contributions.size())
		{
			throw std::logic_error("Cannot add commands to sealed render pass '" + _debugName + "'");
		}
		if (p_command == nullptr)
		{
			throw std::invalid_argument("Cannot add a null command to render pass '" + _debugName + "'");
		}
		_contributions[p_contribution].commands.push_back(std::move(p_command));
	}

	void RenderPass::_seal()
	{
		if (_sealed)
		{
			return;
		}
		std::stable_sort(_contributions.begin(), _contributions.end(), [](const auto &p_lhs, const auto &p_rhs) {
			if (p_lhs.priority != p_rhs.priority)
			{
				return p_lhs.priority < p_rhs.priority;
			}
			if (p_lhs.registrationOrder != p_rhs.registrationOrder)
			{
				return p_lhs.registrationOrder < p_rhs.registrationOrder;
			}
			return p_lhs.declarationOrder < p_rhs.declarationOrder;
		});
		_sealed = true;
	}

	std::vector<std::unique_ptr<spk::RenderCommand>> RenderPass::takeCommands()
	{
		if (_commandsTaken)
		{
			throw std::logic_error("RenderPass commands may only be taken once");
		}
		if (!_sealed)
		{
			throw std::logic_error("RenderPass commands can only be taken after sealing");
		}
		_commandsTaken = true;
		std::vector<std::unique_ptr<spk::RenderCommand>> commands;
		commands.reserve(commandCount());
		for (auto &contribution : _contributions)
		{
			for (auto &command : contribution.commands)
			{
				commands.push_back(std::move(command));
			}
		}
		return commands;
	}

	RenderContributionSink::RenderContributionSink(spk::RenderPass &p_pass, std::size_t p_contribution) noexcept :
		_pass(&p_pass),
		_contribution(p_contribution)
	{
	}

	void RenderContributionSink::add(std::unique_ptr<spk::RenderCommand> p_command)
	{
		if (_pass == nullptr)
		{
			throw std::logic_error("Invalid render contribution sink");
		}
		_pass->_add(_contribution, std::move(p_command));
	}
}
