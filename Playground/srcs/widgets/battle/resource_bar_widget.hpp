#pragma once

#include "battle/presentation/battle_hud_view_model.hpp"
#include "structures/widget/spk_panel.hpp"
#include "structures/widget/spk_text_label.hpp"

namespace pg
{
	class ResourceBarWidget final : public spk::Widget
	{
	private:
		spk::Panel _background;
		spk::Panel _fill;
		spk::TextLabel _label;
		ResourceViewModel _model;

		void _layout();

	protected:
		void _onGeometryChange() override;

	public:
		explicit ResourceBarWidget(const std::string &p_name, spk::Widget *p_parent = nullptr);
		void render(const ResourceViewModel &p_model);
		[[nodiscard]] static float ratio(const ResourceViewModel &p_model) noexcept;
	};
}
