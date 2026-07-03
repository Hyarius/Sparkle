#pragma once

#include "abilities/effect.hpp"

#include <string_view>
#include <vector>

namespace pg
{
	class BattleContext;
	class BattleUnit;
	struct Status;

	struct BattleStatus
	{
		const Status *definition = nullptr;
		int stacks = 0;
		Duration remaining;
		bool isSourcePassive = false;
		int appliedTurnIndex = -1;
	};

	struct BattleStatusRemoval
	{
		const Status *definition = nullptr;
		int stacks = 0;
	};

	class BattleStatuses
	{
	private:
		BattleUnit *_owner = nullptr;
		BattleContext *_context = nullptr;
		std::vector<BattleStatus> _entries;

	public:
		void bind(BattleUnit &p_owner, BattleContext &p_context) noexcept;
		[[nodiscard]] BattleStatusRemoval add(
			const Status &p_status, int p_stackCount, const Duration &p_duration, bool p_isSourcePassive = false);
		[[nodiscard]] BattleStatusRemoval remove(
			std::string_view p_statusId, int p_stackCount, bool p_includeSourcePassives = true);
		[[nodiscard]] BattleStatusRemoval consume(
			std::string_view p_statusId, int p_stackCount, bool p_includeSourcePassives = true);
		[[nodiscard]] std::vector<BattleStatusRemoval> removeByTags(
			const std::vector<std::string> &p_tags, bool p_includeSourcePassives = false);
		[[nodiscard]] bool contains(std::string_view p_statusId, bool p_includeSourcePassives = true) const;
		[[nodiscard]] bool hasTag(std::string_view p_tag, bool p_includeSourcePassives = true) const;
		[[nodiscard]] std::vector<BattleStatusRemoval> advanceTurnDurations(
			bool p_stunOnly = false, bool p_skipAppliedCurrentTurn = false);
		[[nodiscard]] std::vector<BattleStatusRemoval> advanceSeconds(float p_seconds);
		[[nodiscard]] const std::vector<BattleStatus> &entries() const noexcept;
		[[nodiscard]] BattleUnit *owner() const noexcept;
		[[nodiscard]] BattleContext *context() const noexcept;
	};
}
