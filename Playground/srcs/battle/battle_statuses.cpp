#include "battle/battle_statuses.hpp"

#include "battle/battle_context.hpp"
#include "statuses/status.hpp"

#include <algorithm>
#include <cctype>

namespace
{
	std::string normalize(std::string_view p_value)
	{
		std::string result(p_value);
		std::ranges::transform(result, result.begin(), [](unsigned char character) {
			return static_cast<char>(std::tolower(character));
		});
		return result;
	}
}

namespace pg
{
	void BattleStatuses::bind(BattleUnit &p_owner, BattleContext &p_context) noexcept
	{
		_owner = &p_owner;
		_context = &p_context;
	}

	BattleStatusRemoval BattleStatuses::add(
		const Status &p_status, int p_stackCount, const Duration &p_duration, bool p_isSourcePassive)
	{
		if (p_stackCount <= 0)
		{
			return {};
		}
		Duration duration = p_duration;
		if (p_isSourcePassive)
		{
			duration = {};
		}
		const int turnIndex = _context != nullptr ? _context->currentTurn.turnIndex : -1;
		_entries.push_back({&p_status, p_stackCount, duration, p_isSourcePassive, turnIndex});
		return {&p_status, p_stackCount};
	}

	BattleStatusRemoval BattleStatuses::remove(
		std::string_view p_statusId, int p_stackCount, bool p_includeSourcePassives)
	{
		BattleStatusRemoval result;
		if (p_stackCount <= 0)
		{
			return result;
		}
		int remaining = p_stackCount;
		for (auto entry = _entries.begin(); entry != _entries.end() && remaining > 0;)
		{
			if (entry->definition == nullptr || entry->definition->id != p_statusId ||
				(!p_includeSourcePassives && entry->isSourcePassive))
			{
				++entry;
				continue;
			}
			const int removed = std::min(entry->stacks, remaining);
			result.definition = entry->definition;
			result.stacks += removed;
			entry->stacks -= removed;
			remaining -= removed;
			entry = entry->stacks == 0 ? _entries.erase(entry) : std::next(entry);
		}
		return result;
	}

	BattleStatusRemoval BattleStatuses::consume(
		std::string_view p_statusId, int p_stackCount, bool p_includeSourcePassives)
	{
		return remove(p_statusId, p_stackCount, p_includeSourcePassives);
	}

	std::vector<BattleStatusRemoval> BattleStatuses::removeByTags(
		const std::vector<std::string> &p_tags, bool p_includeSourcePassives)
	{
		std::vector<BattleStatusRemoval> result;
		for (auto entry = _entries.begin(); entry != _entries.end();)
		{
			const bool matches = entry->definition != nullptr &&
								 (p_includeSourcePassives || !entry->isSourcePassive) &&
								 std::ranges::any_of(p_tags, [&](const std::string &tag) {
									 return std::ranges::any_of(entry->definition->tags, [&](const std::string &candidate) {
										 return normalize(candidate) == normalize(tag);
									 });
								 });
			if (!matches)
			{
				++entry;
				continue;
			}
			result.push_back({entry->definition, entry->stacks});
			entry = _entries.erase(entry);
		}
		return result;
	}

	bool BattleStatuses::contains(std::string_view p_statusId, bool p_includeSourcePassives) const
	{
		return std::ranges::any_of(_entries, [&](const BattleStatus &entry) {
			return entry.definition != nullptr && entry.definition->id == p_statusId &&
				   (p_includeSourcePassives || !entry.isSourcePassive);
		});
	}

	bool BattleStatuses::hasTag(std::string_view p_tag, bool p_includeSourcePassives) const
	{
		const std::string expected = normalize(p_tag);
		return !expected.empty() && std::ranges::any_of(_entries, [&](const BattleStatus &entry) {
			return entry.definition != nullptr && (p_includeSourcePassives || !entry.isSourcePassive) &&
				   std::ranges::any_of(entry.definition->tags, [&](const std::string &tag) {
					   return normalize(tag) == expected;
				   });
		});
	}

	std::vector<BattleStatusRemoval> BattleStatuses::advanceTurnDurations(
		bool p_stunOnly, bool p_skipAppliedCurrentTurn)
	{
		std::vector<BattleStatusRemoval> result;
		for (auto entry = _entries.begin(); entry != _entries.end();)
		{
			const bool isStun = entry->definition != nullptr &&
								std::ranges::find(entry->definition->tags, "stun") != entry->definition->tags.end();
			const int currentTurn = _context != nullptr ? _context->currentTurn.turnIndex : -2;
			if (entry->remaining.kind != DurationKind::TurnBased || (p_stunOnly && !isStun) ||
				(p_skipAppliedCurrentTurn && entry->appliedTurnIndex == currentTurn))
			{
				++entry;
				continue;
			}
			if (--entry->remaining.turns > 0)
			{
				++entry;
				continue;
			}
			result.push_back({entry->definition, entry->stacks});
			entry = _entries.erase(entry);
		}
		return result;
	}

	std::vector<BattleStatusRemoval> BattleStatuses::advanceSeconds(float p_seconds)
	{
		std::vector<BattleStatusRemoval> result;
		if (p_seconds <= 0.0f)
		{
			return result;
		}
		for (auto entry = _entries.begin(); entry != _entries.end();)
		{
			if (entry->remaining.kind != DurationKind::Seconds)
			{
				++entry;
				continue;
			}
			entry->remaining.seconds -= p_seconds;
			if (entry->remaining.seconds > 0.0f)
			{
				++entry;
				continue;
			}
			result.push_back({entry->definition, entry->stacks});
			entry = _entries.erase(entry);
		}
		return result;
	}

	const std::vector<BattleStatus> &BattleStatuses::entries() const noexcept
	{
		return _entries;
	}
	BattleUnit *BattleStatuses::owner() const noexcept
	{
		return _owner;
	}
	BattleContext *BattleStatuses::context() const noexcept
	{
		return _context;
	}
}
