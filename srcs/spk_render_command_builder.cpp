#include "spk_render_command_builder.hpp"

namespace spk
{
	void RenderCommandBuilder::clear()
	{
		_commands.clear();
	}

	bool RenderCommandBuilder::empty() const
	{
		return (_commands.empty());
	}

	size_t RenderCommandBuilder::size() const
	{
		return (_commands.size());
	}

	const std::vector<std::unique_ptr<spk::RenderCommand>>& RenderCommandBuilder::commands() const
	{
		return (_commands);
	}
}