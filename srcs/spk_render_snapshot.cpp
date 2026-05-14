#include "spk_render_snapshot.hpp"

namespace spk
{
	RenderSnapshot::RenderSnapshot(std::vector<std::shared_ptr<const spk::RenderUnit>>&& p_units) :
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

	const std::vector<std::shared_ptr<const spk::RenderUnit>>& RenderSnapshot::units() const
	{
		return (_units);
	}

	void RenderSnapshot::execute() const
	{
		for (const std::shared_ptr<const spk::RenderUnit>& unit : _units)
		{
			if (unit != nullptr)
			{
				unit->execute();
			}
		}
	}
}
