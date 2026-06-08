#include "structures/system/device/window/spk_window_snapshot_manager.hpp"

#include <memory>
#include <utility>

#include "structures/graphics/opengl/spk_opengl_clear_command.hpp"
#include "structures/graphics/rendering/command/spk_viewport_render_command.hpp"
#include "structures/graphics/rendering/snapshot/spk_render_snapshot_builder.hpp"
#include "structures/graphics/rendering/unit/spk_render_unit_builder.hpp"

namespace spk
{
	namespace
	{
		std::shared_ptr<spk::RenderUnit> _createRenderPassPreparationUnit(const spk::Viewport& p_viewport)
		{
			spk::RenderUnitBuilder builder;
			builder.emplace<spk::ViewportCommand>(p_viewport);
			builder.emplace<spk::ClearCommand>();
			return std::make_shared<spk::RenderUnit>(builder.build());
		}
	}

	void WindowSnapshotManager::rebuild(const spk::Widget& p_rootWidget, spk::RenderModule& p_renderModule)
	{
		spk::RenderSnapshotBuilder builder;
		builder.append(_createRenderPassPreparationUnit(p_rootWidget.viewport()));
		p_rootWidget.appendRenderUnits(builder);

		auto snapshot = std::make_shared<spk::RenderSnapshot>(builder.build());
		p_renderModule.publishSnapshot(std::move(snapshot));
	}
}
