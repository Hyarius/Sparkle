#pragma once

#include "battle/battle_side.hpp"
#include "feats/battle_condition.hpp"
#include "structures/container/spk_uuid.hpp"
#include "structures/design_pattern/spk_contract_provider.hpp"
#include "structures/widget/spk_panel.hpp"
#include "structures/widget/spk_push_button.hpp"
#include "structures/widget/spk_text_label.hpp"

#include <functional>
#include <string>
#include <vector>

namespace pg
{
	class BattleContext;
	class CreatureUnit;

	struct FeatRequirementSnapshot
	{
		spk::UUID uuid;
		BattleCondition::Advancement advancement;
	};

	struct FeatNodeSnapshot
	{
		spk::UUID uuid;
		int completionCount = 0;
		std::vector<FeatRequirementSnapshot> requirements;
	};

	struct CreatureFeatSnapshot
	{
		CreatureUnit *creature = nullptr;
		std::vector<FeatNodeSnapshot> nodes;
	};

	struct FeatSummaryRow
	{
		std::string creatureName;
		std::vector<std::string> progressedNodes;
		std::vector<std::string> completedNodes;
	};

	class BattleResultScreen : public spk::Panel
	{
	private:
		spk::TextLabel _title;
		spk::TextLabel _summary;
		spk::PushButton _continue;
		spk::PushButton::Contract _continueContract;
		BattleContext *_context = nullptr;
		std::vector<CreatureFeatSnapshot> _snapshots;
		std::vector<FeatSummaryRow> _featRows;
		std::vector<std::string> _recruits;
		spk::ContractProvider<CreatureUnit *, int>::Contract _featContract;
		spk::ContractProvider<CreatureUnit *>::Contract _recruitContract;
		std::function<void()> _confirmation;
		BattleSide _winner = BattleSide::Neutral;

		void _recordFeat(CreatureUnit *p_creature);
		void _refreshText();
		void _confirm();

	protected:
		void _onGeometryChange() override;

	public:
		explicit BattleResultScreen(const std::string &p_name, spk::Widget *p_parent = nullptr);

		static CreatureFeatSnapshot captureFeatProgress(CreatureUnit &p_creature);
		static FeatSummaryRow assembleFeatSummary(
			const CreatureUnit &p_creature, const CreatureFeatSnapshot &p_before);
		void bind(BattleContext &p_context);
		void show(BattleSide p_winner, std::function<void()> p_confirmation);
		void hide();
		void unbind();
		void confirm();
		[[nodiscard]] const std::vector<FeatSummaryRow> &featRows() const noexcept;
		[[nodiscard]] const std::vector<std::string> &recruits() const noexcept;
		[[nodiscard]] const spk::Font::Text &summaryText() const;
	};
}
