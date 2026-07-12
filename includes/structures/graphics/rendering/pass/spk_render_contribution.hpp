#pragma once

#include <concepts>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <utility>
#include <vector>

#include "structures/graphics/rendering/command/spk_render_command.hpp"

namespace spk
{
	class RenderPass;

	struct RenderContribution
	{
		std::int32_t priority = 0;
		std::size_t registrationOrder = 0;
		std::size_t declarationOrder = 0;
		std::vector<std::unique_ptr<spk::RenderCommand>> commands;
	};

	class RenderContributionSink
	{
	private:
		spk::RenderPass *_pass = nullptr;
		std::size_t _contribution = 0;

		RenderContributionSink(spk::RenderPass &p_pass, std::size_t p_contribution) noexcept;
		friend class spk::RenderPass;

	public:
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
	};

	namespace RenderContributionPriorities
	{
		inline constexpr std::int32_t PassSetup = -100000;
		inline constexpr std::int32_t Default = 0;
	}
}
