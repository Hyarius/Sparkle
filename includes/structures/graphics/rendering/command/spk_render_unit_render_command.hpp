#pragma once

#include <memory>

#include "structures/graphics/rendering/command/spk_render_command.hpp"

namespace spk
{
	class RenderUnit;

	class RenderUnitRenderCommand : public spk::RenderCommand
	{
	private:
		std::shared_ptr<spk::RenderUnit> _unit;

	public:
		explicit RenderUnitRenderCommand(std::shared_ptr<spk::RenderUnit> p_unit);
		[[nodiscard]] const std::shared_ptr<spk::RenderUnit> &unit() const noexcept;
		void execute(spk::RenderContext &p_context) override;
	};
}
