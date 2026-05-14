#pragma once

#include <memory>
#include <mutex>

#include "spk_render_snapshot.hpp"

namespace spk
{
	class RenderModule
	{
	private:
		mutable std::mutex _snapshotMutex;
		std::shared_ptr<const spk::RenderSnapshot> _currentSnapshot = nullptr;

	public:
		RenderModule();

		void publishSnapshot(std::shared_ptr<const spk::RenderSnapshot> p_snapshot);
		[[nodiscard]] std::shared_ptr<const spk::RenderSnapshot> currentSnapshot() const;

		void render() const;
	};
}
