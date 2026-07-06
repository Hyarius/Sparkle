#pragma once

#include "structures/widget/spk_panel.hpp"
#include "structures/widget/spk_push_button.hpp"
#include "structures/widget/spk_text_label.hpp"
#include "ui/ability_slot_widget.hpp"

#include <array>
#include <functional>
#include <memory>

namespace pg
{
	class BattleContext;
	class BattleUnit;
	struct Ability;

	class AbilityBarWidget : public spk::Panel
	{
	public:
		static constexpr std::size_t PageSize = 8;

	private:
		std::array<std::unique_ptr<AbilitySlotWidget>, PageSize> _slots;
		spk::PushButton _previous;
		spk::PushButton _next;
		spk::TextLabel _pageLabel;
		spk::PushButton::Contract _previousContract;
		spk::PushButton::Contract _nextContract;
		const BattleContext *_context = nullptr;
		BattleUnit *_unit = nullptr;
		const Ability *_selected = nullptr;
		std::size_t _page = 0;
		std::function<void(std::size_t)> _selection;

	protected:
		void _onGeometryChange() override;

	public:
		AbilityBarWidget(
			const std::string &p_name,
			std::shared_ptr<spk::SpriteSheet> p_icons,
			spk::Widget *p_parent = nullptr);

		static std::size_t pageCountFor(std::size_t p_abilityCount) noexcept;
		void bind(const BattleContext *p_context, BattleUnit *p_unit);
		void unbind();
		void setSelectionHandler(std::function<void(std::size_t)> p_selection);
		void setHoverHandler(std::function<void(const Ability *)> p_hover);
		void setSelected(const Ability *p_selected);
		void setPage(std::size_t p_page);
		void refresh();

		[[nodiscard]] std::size_t page() const noexcept;
		[[nodiscard]] std::size_t pageCount() const noexcept;
		[[nodiscard]] const AbilitySlotWidget &slot(std::size_t p_index) const;
	};
}
