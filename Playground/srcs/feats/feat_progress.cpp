#include "feats/feat_progress.hpp"

#include "feats/feat_board.hpp"
#include "feats/uuid.hpp"
#include "creatures/apply_progress.hpp"
#include "creatures/creature_species.hpp"
#include "creatures/creature_unit.hpp"

#include <algorithm>
#include <iostream>

namespace pg
{
	bool FeatRequirementProgress::persistsAcrossFights() const noexcept
	{
		return condition != nullptr && condition->scope == BattleCondition::Scope::Game;
	}

	bool FeatNodeProgress::isCompleted() const noexcept
	{
		return std::ranges::all_of(requirements, [](const FeatRequirementProgress &p_progress) {
			return p_progress.condition != nullptr && p_progress.condition->isComplete(p_progress.advancement);
		});
	}

	bool FeatNodeProgress::isExhausted(const FeatNode &p_node) const noexcept
	{
		return p_node.repeatLimit > 0 && completionCount >= p_node.repeatLimit;
	}

	void FeatNodeProgress::complete()
	{
		++completionCount;
		for (FeatRequirementProgress &progress : requirements) progress.advancement = {};
	}

	void FeatNodeProgress::resetTransientRequirementProgress() noexcept
	{
		for (FeatRequirementProgress &progress : requirements) if (!progress.persistsAcrossFights()) progress.advancement = {};
	}

	FeatNodeProgress *FeatBoardProgress::findProgress(const spk::UUID &p_nodeUuid) noexcept
	{
		for (FeatNodeProgress &progress : nodes) if (progress.nodeUuid == p_nodeUuid) return &progress;
		return nullptr;
	}

	const FeatNodeProgress *FeatBoardProgress::findProgress(const spk::UUID &p_nodeUuid) const noexcept
	{
		for (const FeatNodeProgress &progress : nodes) if (progress.nodeUuid == p_nodeUuid) return &progress;
		return nullptr;
	}

	FeatNodeProgress &FeatBoardProgress::getOrCreateProgress(const FeatNode &p_node)
	{
		if (FeatNodeProgress *found = findProgress(p_node.uuid); found != nullptr) return *found;
		FeatNodeProgress result; result.nodeUuid = p_node.uuid;
		for (const FeatRequirementEntry &entry : p_node.requirements)
			result.requirements.push_back({entry.uuid, entry.condition.get(), {}});
		nodes.push_back(std::move(result)); return nodes.back();
	}

	nlohmann::json FeatBoardProgress::toJson() const
	{
		nlohmann::json result = nlohmann::json::object();
		for (const FeatNodeProgress &node : nodes)
		{
			nlohmann::json requirements = nlohmann::json::object();
			for (const FeatRequirementProgress &requirement : node.requirements)
			{
				requirements[requirement.requirementUuid.toString()] = {
					{"progress", requirement.advancement.progress},
					{"completedRepeatCount", requirement.advancement.completedRepeatCount}};
			}
			result[node.nodeUuid.toString()] = {{"completionCount", node.completionCount}, {"requirements", std::move(requirements)}};
		}
		return result;
	}

	std::size_t FeatBoardProgress::fromJson(const nlohmann::json &p_json, const FeatBoard &p_board)
	{
		nodes.clear(); std::size_t warnings = 0;
		if (!p_json.is_object()) throw std::invalid_argument("feat progress JSON must be an object");
		for (const auto &[nodeId, value] : p_json.items())
		{
			const std::optional<spk::UUID> uuid = uuidFromString(nodeId);
			const FeatNode *node = uuid.has_value() ? p_board.tryNode(*uuid) : nullptr;
			if (node == nullptr)
			{
				++warnings; std::cerr << "FeatBoardProgress: dropping unknown node UUID '" << nodeId << "'" << std::endl; continue;
			}
			if (!value.is_object()) throw std::invalid_argument("feat node progress must be an object");
			FeatNodeProgress &progress = getOrCreateProgress(*node);
			progress.completionCount = std::max(0, value.value("completionCount", 0));
			const auto requirementsIt = value.find("requirements");
			if (requirementsIt == value.end()) continue;
			if (!requirementsIt->is_object()) throw std::invalid_argument("feat requirement progress must be an object");
			for (const auto &[requirementId, requirementValue] : requirementsIt->items())
			{
				const std::optional<spk::UUID> requirementUuid = uuidFromString(requirementId);
				auto found = requirementUuid.has_value() ? std::ranges::find_if(progress.requirements, [&](const FeatRequirementProgress &p_entry) { return p_entry.requirementUuid == *requirementUuid; }) : progress.requirements.end();
				if (found == progress.requirements.end())
				{
					++warnings; std::cerr << "FeatBoardProgress: dropping unknown requirement UUID '" << requirementId << "'" << std::endl; continue;
				}
				if (!requirementValue.is_object()) throw std::invalid_argument("feat requirement advancement must be an object");
				found->advancement.progress = std::clamp(requirementValue.value("progress", 0.0f), 0.0f, 99.999f);
				found->advancement.completedRepeatCount = std::clamp(requirementValue.value("completedRepeatCount", 0), 0, found->condition->requiredRepeatCount);
			}
		}
		return warnings;
	}

	void FeatBoardProgress::fromJson(const nlohmann::json &p_json)
	{
		nodes.clear();
		if (!p_json.is_object()) throw std::invalid_argument("feat progress JSON must be an object");
		for (const auto &[nodeId, value] : p_json.items())
		{
			const std::optional<spk::UUID> uuid = uuidFromString(nodeId);
			if (!uuid.has_value() || !value.is_object()) continue;
			nodes.push_back({.nodeUuid = *uuid, .completionCount = std::max(0, value.value("completionCount", 0))});
		}
	}

	void FeatBoardProgress::resetTransientRequirementProgress() noexcept
	{
		for (FeatNodeProgress &progress : nodes) progress.resetTransientRequirementProgress();
	}

	bool completeNodeDirect(CreatureUnit &p_unit, const spk::UUID &p_nodeUuid)
	{
		if (p_unit.species == nullptr || p_unit.species->featBoard == nullptr) return false;
		const FeatNode *node = p_unit.species->featBoard->tryNode(p_nodeUuid);
		if (node == nullptr) return false;
		FeatNodeProgress &progress = p_unit.featBoardProgress.getOrCreateProgress(*node);
		if (progress.isExhausted(*node)) return false;
		progress.complete(); applyProgress(p_unit); return true;
	}
}
