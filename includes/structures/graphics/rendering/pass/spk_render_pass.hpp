#pragma once

#include <concepts>
#include <cstddef>
#include <memory>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "structures/design_pattern/spk_priorizable_trait.hpp"
#include "structures/graphics/rendering/command/spk_render_command.hpp"
#include "structures/graphics/rendering/pass/spk_render_pass_clear.hpp"
#include "structures/graphics/rendering/pass/spk_render_target_reference.hpp"

namespace spk
{
	class RenderPipeline;

	class RenderPass : public spk::PriorizableTrait
	{
	public:
		struct Description
		{
			spk::RenderTargetReference target;
			spk::RenderPassClear clear;
		};

	private:
		std::string _id;
		Description _description;
		std::vector<std::unique_ptr<spk::RenderCommand>> _commands;
		bool _sealed = false;
		bool _commandsTaken = false;

		void _seal() noexcept;
		void _ensureMutable() const;
		friend class spk::RenderPipeline;

	public:
		RenderPass(std::string p_id, Priority p_priority, Description p_description);
		virtual ~RenderPass() = default;

		RenderPass(const RenderPass &) = delete;
		RenderPass &operator=(const RenderPass &) = delete;
		RenderPass(RenderPass &&) noexcept = delete;
		RenderPass &operator=(RenderPass &&) noexcept = delete;

		[[nodiscard]] const std::string &id() const noexcept;
		[[nodiscard]] const Description &description() const noexcept;
		[[nodiscard]] std::size_t commandCount() const noexcept;

		void add(std::unique_ptr<spk::RenderCommand> p_command);

		template <typename TCommand, typename... TArguments>
			requires std::derived_from<TCommand, spk::RenderCommand>
		TCommand &emplace(TArguments &&...p_arguments)
		{
			auto command = std::make_unique<TCommand>(std::forward<TArguments>(p_arguments)...);
			TCommand &result = *command;
			add(std::move(command));
			return result;
		}

		[[nodiscard]] std::vector<std::unique_ptr<spk::RenderCommand>> takeCommands();
	};

	void validateRenderPassDescription(
		const spk::RenderPass::Description &p_description,
		std::string_view p_passId = {});
}
