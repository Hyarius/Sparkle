#include "spk_render_unit_builder.hpp"

namespace spk
{
	void RenderUnitBuilder::clear()
	{
		_commands.clear();
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
