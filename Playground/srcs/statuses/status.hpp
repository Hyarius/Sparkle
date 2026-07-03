#pragma once

#include <array>
#include <filesystem>
#include <memory>
#include <string>
#include <vector>

namespace pg
{
	class Effect;
	class JsonReader;
	template <typename TDefinition>
	class Registry;

	enum class StatusHookPoint
	{
		TurnStart,
		TurnEnd,
		BeforeConsumingResources,
		AfterConsumingResources,
		OnHPLoss,
		OnAPLoss,
		OnMPLoss,
		OnHPGain,
		OnAPGain,
		OnMPGain
	};

	struct Status
	{
		std::string id;
		std::string displayName;
		std::array<int, 2> icon{};
		std::vector<std::string> tags;
		StatusHookPoint hookPoint = StatusHookPoint::TurnStart;
		std::vector<std::shared_ptr<const Effect>> effects;
	};

	[[nodiscard]] Status parseStatus(JsonReader &p_reader);
	void resolveStatusReferences(
		Status &p_status, const Registry<Status> &p_statuses, const std::filesystem::path &p_file);
}
