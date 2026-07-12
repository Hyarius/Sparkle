#include "structures/graphics/rendering/pipeline/spk_render_pass.hpp"

#include <algorithm>
#include <cmath>
#include <sstream>
#include <stdexcept>
#include <unordered_set>

#include "structures/graphics/spk_framebuffer_object.hpp"
#include "structures/graphics/spk_texture.hpp"

namespace spk
{
	namespace
	{
		[[nodiscard]] bool isDrawPhase(RenderPhase p_phase)
		{
			return p_phase != RenderPhase::PassSetup && p_phase < RenderPhase::Count;
		}
	}

	std::string renderPassDisplayName(const RenderPassId &p_id)
	{
		if (p_id.debugName.empty() == false)
		{
			return p_id.debugName;
		}
		switch (p_id.kind)
		{
		case RenderPassKind::MainScene:
			return "MainScene";
		case RenderPassKind::Shadow:
			return "Shadow[" + std::to_string(p_id.instance) + "]";
		case RenderPassKind::Auxiliary:
			return "Auxiliary[" + std::to_string(p_id.instance) + "]";
		}
		return "Invalid";
	}

	void validateRenderPassDescription(const RenderPassDescription &p_description)
	{
		const std::string name = renderPassDisplayName(p_description.id);
		if (p_description.id.kind == RenderPassKind::MainScene && p_description.id.instance != 0)
		{
			throw std::invalid_argument("Render pass '" + name + "': MainScene instance must be zero");
		}
		if (p_description.phases.empty())
		{
			throw std::invalid_argument("Render pass '" + name + "': phase list cannot be empty");
		}
		if (p_description.target.viewport.geometry().empty() || p_description.target.viewport.scissor().empty())
		{
			throw std::invalid_argument("Render pass '" + name + "': viewport and scissor must be non-empty");
		}

		std::unordered_set<unsigned> phases;
		bool sawDraw = false;
		bool sawOpaque = false;
		for (RenderPhase phase : p_description.phases)
		{
			if (phase >= RenderPhase::Count || !phases.insert(static_cast<unsigned>(phase)).second)
			{
				throw std::invalid_argument("Render pass '" + name + "': invalid or duplicate phase");
			}
			if (phase == RenderPhase::PassSetup && sawDraw)
			{
				throw std::invalid_argument("Render pass '" + name + "': PassSetup must precede draw phases");
			}
			if (phase == RenderPhase::SceneTransparent && !sawOpaque &&
				std::ranges::find(p_description.phases, RenderPhase::SceneOpaque) != p_description.phases.end())
			{
				throw std::invalid_argument("Render pass '" + name + "': SceneOpaque must precede SceneTransparent");
			}
			sawDraw = sawDraw || isDrawPhase(phase);
			sawOpaque = sawOpaque || phase == RenderPhase::SceneOpaque;
		}
		if (p_description.id.kind == RenderPassKind::MainScene && phases.contains(static_cast<unsigned>(RenderPhase::ShadowCaster)))
		{
			throw std::invalid_argument("Render pass '" + name + "': main pass cannot contain ShadowCaster");
		}

		if (p_description.clear.depth.has_value() &&
			(!std::isfinite(*p_description.clear.depth) || *p_description.clear.depth < 0.0f || *p_description.clear.depth > 1.0f))
		{
			throw std::invalid_argument("Render pass '" + name + "': depth clear must be finite and normalized");
		}

		const FrameBufferObject *target = p_description.target.frameBuffer;
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

	RenderPass::RenderPass(RenderPassDescription p_description) :
		_description(std::move(p_description))
	{
		validateRenderPassDescription(_description);
		for (RenderPhase phase : _description.phases)
		{
			_phaseCommands.push_back(PhaseCommands{.phase = phase});
		}
	}

	RenderPass::PhaseCommands &RenderPass::_commandsFor(RenderPhase p_phase)
	{
		auto iterator = std::ranges::find(_phaseCommands, p_phase, &PhaseCommands::phase);
		if (iterator == _phaseCommands.end())
		{
			throw std::invalid_argument("Render pass '" + renderPassDisplayName(_description.id) + "' does not declare phase '" + std::string(renderPhaseName(p_phase)) + "'");
		}
		return *iterator;
	}

	const RenderPass::PhaseCommands &RenderPass::_commandsFor(RenderPhase p_phase) const
	{
		return const_cast<RenderPass *>(this)->_commandsFor(p_phase);
	}

	const RenderPassDescription &RenderPass::description() const noexcept
	{
		return _description;
	}
	bool RenderPass::declares(RenderPhase p_phase) const noexcept
	{
		return std::ranges::find(_description.phases, p_phase) != _description.phases.end();
	}
	std::size_t RenderPass::commandCount(RenderPhase p_phase) const
	{
		return _commandsFor(p_phase).commands.size();
	}
	std::size_t RenderPass::contributorCount(RenderPhase p_phase) const
	{
		return _commandsFor(p_phase).contributorCount;
	}
	void RenderPass::recordContributor(RenderPhase p_phase)
	{
		++_commandsFor(p_phase).contributorCount;
	}
	std::vector<std::unique_ptr<spk::RenderCommand>> RenderPass::takeCommands(RenderPhase p_phase)
	{
		return std::move(_commandsFor(p_phase).commands);
	}

	void RenderPass::add(RenderPhase p_phase, std::unique_ptr<spk::RenderCommand> p_command)
	{
		PhaseCommands &commands = _commandsFor(p_phase);
		if (p_command != nullptr)
		{
			commands.commands.push_back(std::move(p_command));
		}
	}
}
