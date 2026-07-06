#pragma once

#include "structures/graphics/texture/spk_sprite_sheet.hpp"
#include "structures/widget/spk_image_label.hpp"
#include "structures/widget/spk_panel.hpp"
#include "structures/widget/spk_text_label.hpp"

#include <memory>
#include <string>

namespace pg
{
	struct Ability;

	class AbilityCardWidget : public spk::Panel
	{
	private:
		std::shared_ptr<spk::SpriteSheet> _icons;
		spk::ImageLabel _icon;
		spk::TextLabel _name;
		spk::TextLabel _metadata;
		spk::TextLabel _rules;
		const Ability *_ability = nullptr;

	protected:
		void _onGeometryChange() override;

	public:
		AbilityCardWidget(
			const std::string &p_name,
			std::shared_ptr<spk::SpriteSheet> p_icons,
			spk::Widget *p_parent = nullptr);

		static std::string metadataText(const Ability &p_ability);
		static std::string rulesText(const Ability &p_ability);
		void bind(const Ability *p_ability);
		[[nodiscard]] const Ability *ability() const noexcept;
	};
}
