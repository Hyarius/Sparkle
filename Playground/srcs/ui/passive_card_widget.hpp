#pragma once

#include "structures/graphics/texture/spk_sprite_sheet.hpp"
#include "structures/widget/spk_image_label.hpp"
#include "structures/widget/spk_panel.hpp"
#include "structures/widget/spk_text_label.hpp"

#include <memory>
#include <string>

namespace pg
{
	struct Status;

	class PassiveCardWidget : public spk::Panel
	{
	private:
		std::shared_ptr<spk::SpriteSheet> _icons;
		spk::ImageLabel _icon;
		spk::TextLabel _name;
		spk::TextLabel _rules;
		const Status *_status = nullptr;

	protected:
		void _onGeometryChange() override;

	public:
		PassiveCardWidget(
			const std::string &p_name,
			std::shared_ptr<spk::SpriteSheet> p_icons,
			spk::Widget *p_parent = nullptr);

		static std::string rulesText(const Status &p_status);
		void bind(const Status *p_status);
		[[nodiscard]] const Status *status() const noexcept;
	};
}
