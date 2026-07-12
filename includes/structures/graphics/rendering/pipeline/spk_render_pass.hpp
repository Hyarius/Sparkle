#pragma once

#include <cstddef>
#include <memory>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

#include "structures/graphics/rendering/command/spk_render_command.hpp"
#include "structures/graphics/rendering/pipeline/spk_render_frame_context.hpp"

namespace spk
{
	enum class RenderPassKind
	{
		MainScene,
		Shadow,
		Auxiliary
	};

	struct RenderPassId
	{
		RenderPassKind kind = RenderPassKind::Auxiliary;
		std::size_t instance = 0;
		std::string debugName;

		[[nodiscard]] bool operator==(const RenderPassId &p_other) const noexcept
		{
			return kind == p_other.kind && instance == p_other.instance;
		}
	};

	struct RenderPass::Description
	{
		RenderPassId id;
		RenderTargetReference target;
		RenderPassClear clear;
		std::vector<RenderPhase> phases;
	};

	struct RenderPhaseDiagnostics
	{
		RenderPhase phase = RenderPhase::PassSetup;
		std::size_t contributorCount = 0;
		std::size_t commandCount = 0;
	};

	class RenderPass
	{
		friend class RenderPlan;

	private:
		struct PhaseCommands
		{
			RenderPhase phase;
			std::vector<std::unique_ptr<spk::RenderCommand>> commands;
			std::size_t contributorCount = 0;
		};

		RenderPass::Description _description;
		std::vector<PhaseCommands> _phaseCommands;

		[[nodiscard]] PhaseCommands &_commandsFor(RenderPhase p_phase);
		[[nodiscard]] const PhaseCommands &_commandsFor(RenderPhase p_phase) const;

	public:
		explicit RenderPass(RenderPass::Description p_description);

		RenderPass(const RenderPass &) = delete;
		RenderPass &operator=(const RenderPass &) = delete;
		RenderPass(RenderPass &&) noexcept = default;
		RenderPass &operator=(RenderPass &&) noexcept = delete;

		[[nodiscard]] const RenderPass::Description &description() const noexcept;
		[[nodiscard]] bool declares(RenderPhase p_phase) const noexcept;
		[[nodiscard]] std::size_t commandCount(RenderPhase p_phase) const;
		[[nodiscard]] std::size_t contributorCount(RenderPhase p_phase) const;
		void recordContributor(RenderPhase p_phase);
		[[nodiscard]] std::vector<std::unique_ptr<spk::RenderCommand>> takeCommands(RenderPhase p_phase);

		void add(RenderPhase p_phase, std::unique_ptr<spk::RenderCommand> p_command);

		template <typename TCommand, typename... TArguments>
		TCommand &emplace(RenderPhase p_phase, TArguments &&...p_arguments)
		{
			static_assert(std::is_base_of_v<spk::RenderCommand, TCommand>, "TCommand must inherit RenderCommand");
			auto command = std::make_unique<TCommand>(std::forward<TArguments>(p_arguments)...);
			TCommand &result = *command;
			add(p_phase, std::move(command));
			return result;
		}
	};

	void validateRenderPassDescription(const RenderPass::Description &p_description);
	[[nodiscard]] std::string renderPassDisplayName(const RenderPassId &p_id);
}
