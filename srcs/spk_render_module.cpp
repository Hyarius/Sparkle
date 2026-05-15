#include "spk_render_module.hpp"

#include "spk_render_context.hpp"

namespace spk
{
	RenderModule::RenderModule() = default;

	void RenderModule::publishSnapshot(std::shared_ptr<spk::RenderSnapshot> p_snapshot)
	{
		std::scoped_lock lock(_snapshotMutex);
		_currentSnapshot = std::move(p_snapshot);
	}

	std::shared_ptr<spk::RenderSnapshot> RenderModule::currentSnapshot() const
	{
		std::scoped_lock lock(_snapshotMutex);
		return (_currentSnapshot);
	}

	void RenderModule::render(spk::IRenderContext& p_renderContext) const
	{
		std::shared_ptr<spk::RenderSnapshot> snapshot = currentSnapshot();

		if (snapshot == nullptr)
		{
			return;
		}

		snapshot->execute(p_renderContext);
	}
}
