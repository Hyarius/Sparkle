#pragma once

#include "feats/battle_condition.hpp"

#include <span>
#include <vector>

namespace pg
{
	struct BattleEvent;
	struct TamingProfile;

	class TamingProgress
	{
	private:
		const TamingProfile *_profile = nullptr;
		bool _failed = false;

	public:
		std::vector<BattleCondition::Advancement> advancements;

		TamingProgress() = default;
		explicit TamingProgress(const TamingProfile &p_profile);

		[[nodiscard]] bool isImpressed() const noexcept;
		[[nodiscard]] bool hasFailed() const noexcept;
		[[nodiscard]] const TamingProfile *profile() const noexcept;
		void evaluateEvents(std::span<const BattleEvent *const> p_events);
		void markFailed() noexcept;
		void reset() noexcept;
	};
}
