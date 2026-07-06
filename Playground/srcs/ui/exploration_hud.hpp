#pragma once

#include "core/event_center.hpp"
#include "structures/widget/spk_panel.hpp"
#include "structures/widget/spk_push_button.hpp"
#include "structures/widget/spk_text_label.hpp"
#include "ui/creature_card_widget.hpp"

#include <array>
#include <functional>
#include <memory>
#include <string>

namespace pg
{
	struct GameContext;

	class ExplorationHud : public spk::Widget
	{
	private:
		spk::Panel _timePanel;
		spk::TextLabel _timeLabel;
		spk::Panel _zonePanel;
		spk::TextLabel _zoneLabel;
		spk::PushButton _saveButton;
		spk::PushButton _quitButton;
		spk::Panel _partyPanel;
		spk::TextLabel _partyTitle;
		std::array<std::unique_ptr<CreatureCardWidget>, 6> _partyCards;
		spk::Panel _promptPanel;
		spk::TextLabel _promptLabel;
		GameContext *_context = nullptr;
		std::function<void()> _quit;
		std::string _zoneText;
		std::string _promptText;
		std::size_t _partyCount = 0;
		spk::PushButton::Contract _saveContract;
		spk::PushButton::Contract _quitContract;
		spk::ContractProvider<>::Contract _worldContract;
		spk::ContractProvider<>::Contract _partyContract;
		spk::ContractProvider<std::string>::Contract _promptContract;
		spk::ContractProvider<bool>::Contract _modeContract;

		void _refreshZone();
		void _refreshParty();
		void _setPrompt(std::string p_prompt);

	protected:
		void _onGeometryChange() override;

	public:
		ExplorationHud(
			const std::string &p_name,
			std::shared_ptr<spk::SpriteSheet> p_icons,
			spk::Widget *p_parent = nullptr);

		void bind(GameContext &p_context, std::function<void()> p_quit = {});
		void unbind();
		[[nodiscard]] bool bound() const noexcept;
		[[nodiscard]] const std::string &zoneText() const noexcept;
		[[nodiscard]] const std::string &promptText() const noexcept;
		[[nodiscard]] std::size_t partyCount() const noexcept;
		[[nodiscard]] const CreatureCardWidget &partyCard(std::size_t p_index) const;
	};
}
