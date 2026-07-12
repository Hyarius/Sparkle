#include "structures/graphics/rendering/command/spk_render_unit_render_command.hpp"

#include <stdexcept>
#include <utility>

#include "structures/graphics/rendering/unit/spk_render_unit.hpp"

namespace spk
{
	RenderUnitRenderCommand::RenderUnitRenderCommand(std::shared_ptr<spk::RenderUnit> p_unit) :
		_unit(std::move(p_unit))
	{
		if (_unit == nullptr)
		{
			throw std::invalid_argument("RenderUnitRenderCommand requires a render unit");
		}
	}

	const std::shared_ptr<spk::RenderUnit> &RenderUnitRenderCommand::unit() const noexcept
	{
		return _unit;
	}

	void RenderUnitRenderCommand::execute(spk::RenderContext &p_context)
	{
		_unit->execute(p_context);
	}
}
