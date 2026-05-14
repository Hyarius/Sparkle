#pragma once

#include <memory>
#include <type_traits>
#include <utility>
#include <vector>

#include "spk_render_unit.hpp"

namespace spk
{
	class RenderUnitBuilder
	{
	private:
		std::vector<std::unique_ptr<spk::RenderCommand>> _commands;

	public:
		RenderUnitBuilder() = default;
		~RenderUnitBuilder() = default;

		RenderUnitBuilder(const RenderUnitBuilder&) = delete;
		RenderUnitBuilder& operator=(const RenderUnitBuilder&) = delete;

		RenderUnitBuilder(RenderUnitBuilder&&) noexcept = default;
		RenderUnitBuilder& operator=(RenderUnitBuilder&&) noexcept = default;

		void clear();
		[[nodiscard]] bool empty() const;
		[[nodiscard]] size_t size() const;

		template <typename TCommand, typename... TArguments>
		TCommand& emplace(TArguments&&... p_arguments)
		{
			static_assert(std::is_base_of_v<spk::RenderCommand, TCommand>, "TCommand must inherit from spk::RenderCommand");

			std::unique_ptr<TCommand> newCommand = std::make_unique<TCommand>(std::forward<TArguments>(p_arguments)...);
			TCommand* result = newCommand.get();

			_commands.emplace_back(std::move(newCommand));

			return (*result);
		}

		[[nodiscard]] spk::RenderUnit build();
	};
}
