#include "spk_render_snapshot.hpp"

#include "spk_render_context.hpp"

namespace spk
{
	RenderSnapshot::RenderSnapshot(std::vector<std::shared_ptr<spk::RenderUnit>>&& p_units) :
		_units(std::move(p_units))
	{
	}

	bool RenderSnapshot::empty() const
	{
		return (_units.empty());
	}

	size_t RenderSnapshot::size() const
	{
		return (_units.size());
	}

	const std::vector<std::shared_ptr<spk::RenderUnit>>& RenderSnapshot::units() const
	{
		return (_units);
	}

	void RenderSnapshot::execute(spk::IRenderContext& p_renderContext)
	{
		for (const std::shared_ptr<spk::RenderUnit>& unit : _units)
		{
			if (unit != nullptr)
			{
				unit->execute(p_renderContext);
			}
		}
	}
}
