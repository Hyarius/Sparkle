#pragma once

#include "feats/battle_condition.hpp"
#include "structures/container/spk_uuid.hpp"

#include <cstddef>
#include <nlohmann/json.hpp>
#include <vector>

namespace pg
{
	class FeatBoard;
	class CreatureUnit;
	struct FeatNode;
	using FeatRequirement = BattleCondition;

	struct FeatRequirementProgress
	{
		spk::UUID requirementUuid;
		const FeatRequirement *condition = nullptr;
		BattleCondition::Advancement advancement;
		[[nodiscard]] bool persistsAcrossFights() const noexcept;
	};

	struct FeatNodeProgress
	{
		spk::UUID nodeUuid;
		int completionCount = 0;
		std::vector<FeatRequirementProgress> requirements;
		[[nodiscard]] bool isCompleted() const noexcept;
		[[nodiscard]] bool isExhausted(const FeatNode &p_node) const noexcept;
		void complete();
		void resetTransientRequirementProgress() noexcept;
	};

	struct FeatBoardProgress
	{
		std::vector<FeatNodeProgress> nodes;

		[[nodiscard]] FeatNodeProgress *findProgress(const spk::UUID &p_nodeUuid) noexcept;
		[[nodiscard]] const FeatNodeProgress *findProgress(const spk::UUID &p_nodeUuid) const noexcept;
		FeatNodeProgress &getOrCreateProgress(const FeatNode &p_node);
		[[nodiscard]] nlohmann::json toJson() const;
		std::size_t fromJson(const nlohmann::json &p_json, const FeatBoard &p_board);
		void fromJson(const nlohmann::json &p_json);
		void resetTransientRequirementProgress() noexcept;
	};

	bool completeNodeDirect(CreatureUnit &p_unit, const spk::UUID &p_nodeUuid);
}
