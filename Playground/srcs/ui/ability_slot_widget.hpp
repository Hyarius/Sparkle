#pragma once

#include "structures/graphics/texture/spk_sprite_sheet.hpp"
#include "structures/widget/spk_push_button.hpp"

#include <functional>
#include <memory>

namespace pg
{
	struct Ability;

	class AbilitySlotWidget : public spk::PushButton
	{
	private:
		std::size_t _slot = 0;
		const Ability *_ability = nullptr;
		bool _enabled = false;
		bool _selected = false;
		std::shared_ptr<spk::SpriteSheet> _icons;
		std::function<void(std::size_t)> _selection;
		std::function<void(const Ability *)> _hover;
		spk::PushButton::Contract _clickContract;

		void _refreshLook();

	protected:
		void _onMouseMovedEvent(spk::MouseMovedEvent &p_event) override;

	public:
		AbilitySlotWidget(
			const std::string &p_name,
			std::size_t p_slot,
			std::shared_ptr<spk::SpriteSheet> p_icons,
			spk::Widget *p_parent = nullptr);

		void setSelectionHandler(std::function<void(std::size_t)> p_selection);
		void setHoverHandler(std::function<void(const Ability *)> p_hover);
		void setAbility(const Ability *p_ability, bool p_enabled, bool p_selected);
		[[nodiscard]] const Ability *ability() const noexcept;
		[[nodiscard]] bool enabled() const noexcept;
		[[nodiscard]] bool selected() const noexcept;
	};
}
