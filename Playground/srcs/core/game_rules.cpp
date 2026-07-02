#include "core/game_rules.hpp"

namespace pg
{
	GameRules parseGameRules(JsonReader &p_reader)
	{
		p_reader.forbidUnknown({"version", "teamMemberCount", "abilityBarSlots", "maxVerticalTraversalGap", "defaultBoardSize", "deploymentStripDepth", "minimumTurnBarDuration", "mitigationScaling", "timeEffectResistanceScaling", "overlayMasks"});

		const int version = p_reader.require<int>("version");
		if (version != 1)
		{
			throw JsonError(
				p_reader.file(),
				p_reader.pathFor("version"),
				"unsupported version " + std::to_string(version) + " (expected 1)");
		}

		JsonReader overlayMasksReader = p_reader.child("overlayMasks");
		overlayMasksReader.forbidUnknown({"deployment", "movement", "path", "abilityRange", "areaOfEffect", "losBlocked", "hovered", "invalid"});

		GameRules result;
		result.teamMemberCount = p_reader.require<int>("teamMemberCount");
		result.abilityBarSlots = p_reader.require<int>("abilityBarSlots");
		result.maxVerticalTraversalGap = p_reader.require<double>("maxVerticalTraversalGap");
		result.defaultBoardSize = p_reader.require<std::array<int, 2>>("defaultBoardSize");
		result.deploymentStripDepth = p_reader.require<int>("deploymentStripDepth");
		result.minimumTurnBarDuration = p_reader.require<double>("minimumTurnBarDuration");
		result.mitigationScaling = p_reader.require<double>("mitigationScaling");
		result.timeEffectResistanceScaling = p_reader.require<double>("timeEffectResistanceScaling");

		for (const std::string &name : {
				 "deployment",
				 "movement",
				 "path",
				 "abilityRange",
				 "areaOfEffect",
				 "losBlocked",
				 "hovered",
				 "invalid"})
		{
			result.overlayMasks.emplace(name, overlayMasksReader.require<std::array<int, 2>>(name));
		}

		return result;
	}
}
