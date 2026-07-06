#include "ui/battle_result_screen.hpp"

#include "battle/battle_context.hpp"
#include "battle/battle_unit.hpp"
#include "core/event_center.hpp"
#include "creatures/creature_species.hpp"
#include "creatures/creature_unit.hpp"
#include "feats/feat_board.hpp"
#include "feats/feat_progress.hpp"

#include <algorithm>
#include <cmath>
#include <sstream>
#include <utility>

namespace
{
	const pg::FeatNodeSnapshot *findNode(
		const pg::CreatureFeatSnapshot &p_snapshot, const spk::UUID &p_uuid)
	{
		const auto found = std::ranges::find_if(p_snapshot.nodes, [&](const pg::FeatNodeSnapshot &p_node) {
			return p_node.uuid == p_uuid;
		});
		return found == p_snapshot.nodes.end() ? nullptr : &*found;
	}

	const pg::FeatRequirementSnapshot *findRequirement(
		const pg::FeatNodeSnapshot *p_node, const spk::UUID &p_uuid)
	{
		if (p_node == nullptr) return nullptr;
		const auto found = std::ranges::find_if(
			p_node->requirements, [&](const pg::FeatRequirementSnapshot &p_requirement) {
				return p_requirement.uuid == p_uuid;
			});
		return found == p_node->requirements.end() ? nullptr : &*found;
	}
}

namespace pg
{
	BattleResultScreen::BattleResultScreen(const std::string &p_name, spk::Widget *p_parent) :
		spk::Panel(p_name, p_parent),
		_title(p_name + "/Title", this),
		_summary(p_name + "/Summary", this),
		_continue(p_name + "/Continue", "Continue", this),
		_continueContract(_continue.subscribeToClick([this] { _confirm(); }))
	{
		_title.setTextSize(spk::Font::Size(30, 2));
		_summary.setTextSize(spk::Font::Size(15, 1));
		deactivate();
	}

	CreatureFeatSnapshot BattleResultScreen::captureFeatProgress(CreatureUnit &p_creature)
	{
		CreatureFeatSnapshot result{.creature = &p_creature};
		for (const FeatNodeProgress &node : p_creature.featBoardProgress.nodes)
		{
			FeatNodeSnapshot snapshot{.uuid = node.nodeUuid, .completionCount = node.completionCount};
			for (const FeatRequirementProgress &requirement : node.requirements)
			{
				snapshot.requirements.push_back({requirement.requirementUuid, requirement.advancement});
			}
			result.nodes.push_back(std::move(snapshot));
		}
		return result;
	}

	FeatSummaryRow BattleResultScreen::assembleFeatSummary(
		const CreatureUnit &p_creature, const CreatureFeatSnapshot &p_before)
	{
		FeatSummaryRow result{.creatureName = p_creature.displayName()};
		if (p_creature.species == nullptr || p_creature.species->featBoard == nullptr) return result;
		for (const FeatNode &definition : p_creature.species->featBoard->nodes)
		{
			const FeatNodeProgress *current = p_creature.featBoardProgress.findProgress(definition.uuid);
			if (current == nullptr) continue;
			const FeatNodeSnapshot *before = findNode(p_before, definition.uuid);
			const int beforeCompletion = before != nullptr ? before->completionCount : 0;
			if (current->completionCount > beforeCompletion)
			{
				result.completedNodes.push_back(definition.displayName);
				continue;
			}
			bool progressed = false;
			for (const FeatRequirementProgress &requirement : current->requirements)
			{
				const FeatRequirementSnapshot *old = findRequirement(before, requirement.requirementUuid);
				const BattleCondition::Advancement previous = old != nullptr
					? old->advancement
					: BattleCondition::Advancement{};
				if (requirement.advancement.completedRepeatCount > previous.completedRepeatCount ||
					requirement.advancement.progress > previous.progress + 0.001f)
				{
					progressed = true;
					break;
				}
			}
			if (progressed) result.progressedNodes.push_back(definition.displayName);
		}
		return result;
	}

	void BattleResultScreen::bind(BattleContext &p_context)
	{
		unbind();
		_context = &p_context;
		for (BattleUnit *unit : _context->getUnits(BattleSide::Player))
		{
			if (unit != nullptr && unit->source() != nullptr)
			{
				_snapshots.push_back(captureFeatProgress(*unit->source()));
			}
		}
		_featContract = _context->events().featProgressUpdated.subscribe(
			[this](CreatureUnit *p_creature, int) { _recordFeat(p_creature); });
		_recruitContract = _context->events().creatureRecruited.subscribe([this](CreatureUnit *p_creature) {
			if (p_creature != nullptr)
			{
				_recruits.push_back(p_creature->displayName());
				_refreshText();
			}
		});
	}

	void BattleResultScreen::_recordFeat(CreatureUnit *p_creature)
	{
		if (p_creature == nullptr) return;
		const auto found = std::ranges::find_if(_snapshots, [&](const CreatureFeatSnapshot &p_snapshot) {
			return p_snapshot.creature == p_creature;
		});
		if (found == _snapshots.end()) return;
		FeatSummaryRow row = assembleFeatSummary(*p_creature, *found);
		const auto existing = std::ranges::find_if(_featRows, [&](const FeatSummaryRow &p_row) {
			return p_row.creatureName == row.creatureName;
		});
		if (existing == _featRows.end()) _featRows.push_back(std::move(row));
		else *existing = std::move(row);
		_refreshText();
	}

	void BattleResultScreen::show(BattleSide p_winner, std::function<void()> p_confirmation)
	{
		_winner = p_winner;
		_confirmation = std::move(p_confirmation);
		_title.setText(p_winner == BattleSide::Player ? "Victory!" : "Defeat");
		_refreshText();
		activate();
	}

	void BattleResultScreen::_refreshText()
	{
		std::ostringstream text;
		if (_featRows.empty()) text << "No feat progress this fight.";
		for (const FeatSummaryRow &row : _featRows)
		{
			text << row.creatureName << ": ";
			for (const std::string &node : row.completedNodes) text << "completed " << node << "; ";
			for (const std::string &node : row.progressedNodes) text << "progressed " << node << "; ";
			text << '\n';
		}
		for (const std::string &recruit : _recruits) text << "Recruited: " << recruit << '\n';
		_summary.setText(text.str());
	}

	void BattleResultScreen::_confirm()
	{
		if (!isActivated()) return;
		auto confirmation = std::move(_confirmation);
		deactivate();
		if (confirmation) confirmation();
	}

	void BattleResultScreen::hide()
	{
		deactivate();
		_confirmation = nullptr;
	}

	void BattleResultScreen::unbind()
	{
		hide();
		_featContract.resign();
		_recruitContract.resign();
		_context = nullptr;
		_snapshots.clear();
		_featRows.clear();
		_recruits.clear();
		_refreshText();
	}

	void BattleResultScreen::confirm() { _confirm(); }

	void BattleResultScreen::_onGeometryChange()
	{
		spk::Panel::_onGeometryChange();
		const unsigned int width = geometry().width();
		const unsigned int height = geometry().height();
		_title.setGeometry(spk::Rect2D(12, 10, width > 24 ? width - 24 : 0, 48));
		_summary.setGeometry(spk::Rect2D(12, 64, width > 24 ? width - 24 : 0, height > 126 ? height - 126 : 0));
		_continue.setGeometry(spk::Rect2D(
			static_cast<int>(width > 132 ? width - 132 : 0),
			static_cast<int>(height > 52 ? height - 52 : 0), 120, 40));
	}

	const std::vector<FeatSummaryRow> &BattleResultScreen::featRows() const noexcept { return _featRows; }
	const std::vector<std::string> &BattleResultScreen::recruits() const noexcept { return _recruits; }
	const spk::Font::Text &BattleResultScreen::summaryText() const { return _summary.text(); }
}
