#pragma once

#include <memory>
#include <utility>
#include <vector>

#include "spk_render_command.hpp"

namespace spk
{
	class RenderUnit
	{
	private:
		std::vector<std::unique_ptr<spk::RenderCommand>> _commands;

	public:
		explicit RenderUnit(std::vector<std::unique_ptr<spk::RenderCommand>>&& p_commands = {});
		~RenderUnit() = default;

		RenderUnit(const RenderUnit&) = delete;
		RenderUnit& operator=(const RenderUnit&) = delete;

		RenderUnit(RenderUnit&&) noexcept = default;
		RenderUnit& operator=(RenderUnit&&) noexcept = default;

		[[nodiscard]] bool empty() const;
		[[nodiscard]] size_t size() const;
		[[nodiscard]] const std::vector<std::unique_ptr<spk::RenderCommand>>& commands() const;

		void execute() const;
	};
}
