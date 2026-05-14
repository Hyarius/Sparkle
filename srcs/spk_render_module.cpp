#include "spk_render_module.hpp"

namespace spk
{
	RenderModule::RenderModule() = default;

	void RenderModule::publishSnapshot(std::shared_ptr<const spk::RenderSnapshot> p_snapshot)
	{
		std::scoped_lock lock(_snapshotMutex);
		_currentSnapshot = std::move(p_snapshot);
	}

	std::shared_ptr<const spk::RenderSnapshot> RenderModule::currentSnapshot() const
	{
		std::scoped_lock lock(_snapshotMutex);
		return (_currentSnapshot);
	}

	void RenderModule::render() const
	{
		std::shared_ptr<const spk::RenderSnapshot> snapshot = currentSnapshot();

		if (snapshot == nullptr)
		{
			return;
		}

		snapshot->execute();
	}
}
