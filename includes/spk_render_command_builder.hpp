#pragma once

#include <memory>
#include <type_traits>
#include <utility>
#include <vector>

#include "spk_render_command.hpp"

namespace spk
{
	class RenderCommandBuilder
	{
	private:
		std::vector<std::unique_ptr<spk::RenderCommand>> _commands;

	public:
		RenderCommandBuilder() = default;
		~RenderCommandBuilder() = default;

		RenderCommandBuilder(const RenderCommandBuilder&) = delete;
		RenderCommandBuilder& operator=(const RenderCommandBuilder&) = delete;

		RenderCommandBuilder(RenderCommandBuilder&&) noexcept = default;
		RenderCommandBuilder& operator=(RenderCommandBuilder&&) noexcept = default;

		void clear();
		[[nodiscard]] bool empty() const;
		[[nodiscard]] size_t size() const;

		[[nodiscard]] const std::vector<std::unique_ptr<spk::RenderCommand>>& commands() const;

		template <typename TCommand, typename... TArguments>
		TCommand& emplace(TArguments&&... p_arguments)
		{
			static_assert(std::is_base_of_v<spk::RenderCommand, TCommand>, "TCommand must inherit from spk::RenderCommand");

			std::unique_ptr<TCommand> newCommand = std::make_unique<TCommand>(std::forward<TArguments>(p_arguments)...);
			TCommand* result = newCommand.get();

			_commands.emplace_back(std::move(newCommand));

			return (*result);
		}
	};
}