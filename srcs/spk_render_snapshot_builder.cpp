#include "spk_render_snapshot_builder.hpp"

namespace spk
{
	void RenderSnapshotBuilder::clear()
	{
		_units.clear();
	}

	void RenderSnapshotBuilder::append(const std::shared_ptr<spk::RenderUnit>& p_unit)
	{
		if (p_unit == nullptr || p_unit->empty() == true)
		{
			return;
		}

		_units.emplace_back(p_unit);
	}

	bool RenderSnapshotBuilder::empty() const
	{
		return (_units.empty());
	}

	size_t RenderSnapshotBuilder::size() const
	{
		return (_units.size());
	}

	const std::vector<std::shared_ptr<spk::RenderUnit>>& RenderSnapshotBuilder::units() const
	{
		return (_units);
	}

	spk::RenderSnapshot RenderSnapshotBuilder::build()
	{
		return spk::RenderSnapshot(std::move(_units));
	}
}
