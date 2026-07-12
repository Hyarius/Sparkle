#include "structures/system/device/window/spk_window_snapshot_manager.hpp"

#include <memory>
#include <utility>

#include "structures/graphics/rendering/snapshot/spk_render_snapshot_builder.hpp"

namespace spk
{
	void WindowSnapshotManager::rebuild(const spk::Widget &p_rootWidget, spk::RenderModule &p_renderModule)
	{
		spk::RenderSnapshotBuilder builder;
		p_rootWidget.appendRenderUnits(
			builder,
			spk::RenderTargetReference{.frameBuffer = nullptr, .viewport = p_rootWidget.viewport(), .activeTarget = false});

		auto snapshot = std::make_shared<spk::RenderSnapshot>(builder.build());
		p_renderModule.publishSnapshot(std::move(snapshot));
	}
}
