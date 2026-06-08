#pragma once

#include "structures/application/module/spk_render_module.hpp"
#include "structures/widget/spk_widget.hpp"

namespace spk
{
	class WindowSnapshotManager
	{
	public:
		void rebuild(const spk::Widget& p_rootWidget, spk::RenderModule& p_renderModule);
	};
}
