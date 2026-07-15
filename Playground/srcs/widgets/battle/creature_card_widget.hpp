#pragma once

#include "widgets/battle/team_roster_card.hpp"

#include "structures/widget/spk_panel.hpp"
#include "structures/widget/spk_push_button.hpp"

#include <memory>
#include <functional>
#include <vector>

namespace pg
{
	class CreatureCardWidget final : public TeamRosterCard
	{
	private:
		spk::PushButton _summaryButton;
		spk::Panel _detailsPanel;
		std::vector<std::unique_ptr<spk::PushButton>> _statistics;
		spk::PushButton::Contract _selectionContract;
		bool _hasCreature = false;
		bool _expanded = false;
		Callback _onSelectedClick;
		Callback _onDetailsClick;

		void _setDetailsVisible(bool p_visible);
		void _applyLayout();

	protected:
		void _onGeometryChange() override;
		void _onMouseButtonReleasedEvent(spk::MouseButtonReleasedEvent &p_event) override;

	public:
		explicit CreatureCardWidget(const std::string &p_name, spk::Widget *p_parent = nullptr);
		void renderCard(const CreatureCardViewModel &p_model, bool p_expanded, bool p_actionable) override;
		[[nodiscard]] std::size_t rosterHeight() const noexcept override;
		void setOnSelectedClick(Callback p_callback) override;
		void setOnDetailsClick(Callback p_callback) override;
	};
}
