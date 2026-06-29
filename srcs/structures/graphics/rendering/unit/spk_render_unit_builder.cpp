#include "structures/graphics/rendering/unit/spk_render_unit_builder.hpp"

namespace spk
{
	void RenderUnitBuilder::clear()
	{
		_commands.clear();
	}

	void RenderUnitBuilder::add(spk::RenderUnit &&p_unit)
	{
		for (std::unique_ptr<spk::RenderCommand> &command : p_unit.takeCommands())
		{
			if (command != nullptr)
			{
				_commands.emplace_back(std::move(command));
			}
		}
	}

	bool RenderUnitBuilder::empty() const
	{
		return (_commands.empty());
	}

	size_t RenderUnitBuilder::size() const
	{
		return (_commands.size());
	}

	spk::RenderUnit RenderUnitBuilder::build()
	{
		return spk::RenderUnit(std::move(_commands));
	}
}
