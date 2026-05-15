#pragma once

#include <memory>
#include <mutex>

#include "spk_render_snapshot.hpp"

namespace spk
{
	class IRenderContext;

	class RenderModule
	{
	private:
		mutable std::mutex _snapshotMutex;
		std::shared_ptr<spk::RenderSnapshot> _currentSnapshot = nullptr;

	public:
		RenderModule();

		void publishSnapshot(std::shared_ptr<spk::RenderSnapshot> p_snapshot);
		[[nodiscard]] std::shared_ptr<spk::RenderSnapshot> currentSnapshot() const;

		void render(spk::IRenderContext& p_renderContext) const;
	};
}
