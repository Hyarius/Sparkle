#include "structures/graphics/rendering/pass/spk_render_pass.hpp"

#include <cmath>
#include <stdexcept>
#include <utility>

#include "structures/graphics/spk_framebuffer_object.hpp"
#include "structures/graphics/spk_texture.hpp"

namespace spk
{
	void validateRenderPassDescription(const spk::RenderPass::Description &p_description, std::string_view p_passId)
	{
		const std::string name = p_passId.empty() ? "<unnamed>" : std::string(p_passId);
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

	RenderPass::RenderPass(std::string p_id, Priority p_priority, Description p_description) :
		PriorizableTrait(p_priority),
		_id(std::move(p_id)),
		_description(std::move(p_description))
	{
		if (_id.empty())
		{
			throw std::invalid_argument("Render pass ID cannot be empty");
		}
		validateRenderPassDescription(_description, _id);
	}

	const std::string &RenderPass::id() const noexcept
	{
		return _id;
	}

	const spk::RenderPass::Description &RenderPass::description() const noexcept
	{
		return _description;
	}

	std::size_t RenderPass::commandCount() const noexcept
	{
		return _commands.size();
	}

	void RenderPass::_ensureMutable() const
	{
		if (_sealed)
		{
			throw std::logic_error("Cannot modify sealed render pass '" + _id + "'");
		}
	}

	void RenderPass::add(std::unique_ptr<spk::RenderCommand> p_command)
	{
		_ensureMutable();
		if (p_command == nullptr)
		{
			throw std::invalid_argument("Cannot add a null command to render pass '" + _id + "'");
		}
		_commands.push_back(std::move(p_command));
	}

	void RenderPass::_seal() noexcept
	{
		_sealed = true;
	}

	std::vector<std::unique_ptr<spk::RenderCommand>> RenderPass::takeCommands()
	{
		if (_commandsTaken)
		{
			throw std::logic_error("Render pass commands may only be taken once");
		}
		if (!_sealed)
		{
			throw std::logic_error("Render pass commands can only be taken after sealing");
		}
		_commandsTaken = true;
		return std::move(_commands);
	}
}
