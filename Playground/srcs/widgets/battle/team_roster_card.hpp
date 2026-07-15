#pragma once

#include "battle/presentation/battle_hud_view_model.hpp"
#include "structures/widget/spk_widget.hpp"

#include <cstddef>
#include <functional>
#include <string>

namespace pg
{
	// Contract implemented by a concrete roster card.  The roster owns ordering, selection and
	// geometry; cards remain free to choose their own visuals and per-card interaction behaviour.
	class TeamRosterCard : public spk::Widget
	{
	public:
		using Callback = std::function<void()>;

		TeamRosterCard(const std::string &p_name, spk::Widget *p_parent) :
			spk::Widget(p_name, p_parent)
		{
		}
		~TeamRosterCard() override = default;

		virtual void renderCard(
			const CreatureCardViewModel &p_model,
			bool p_expanded,
			bool p_actionable) = 0;
		[[nodiscard]] virtual std::size_t rosterHeight() const noexcept = 0;
		virtual void setOnSelectedClick(Callback p_callback) = 0;
		virtual void setOnDetailsClick(Callback p_callback) = 0;
	};
}
