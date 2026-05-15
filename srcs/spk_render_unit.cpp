#include "spk_render_unit.hpp"

#include "spk_render_context.hpp"

namespace spk
{
	RenderUnit::RenderUnit(std::vector<std::unique_ptr<spk::RenderCommand>>&& p_commands) :
		_commands(std::move(p_commands))
	{
	}

	bool RenderUnit::empty() const
	{
		return (_commands.empty());
	}

	size_t RenderUnit::size() const
	{
		return (_commands.size());
	}

	const std::vector<std::unique_ptr<spk::RenderCommand>>& RenderUnit::commands() const
	{
		return (_commands);
	}

	void RenderUnit::execute(spk::IRenderContext& p_renderContext)
	{
		for (const std::unique_ptr<spk::RenderCommand>& command : _commands)
		{
			if (command != nullptr)
			{
				command->execute(p_renderContext);
			}
		}
	}
}
