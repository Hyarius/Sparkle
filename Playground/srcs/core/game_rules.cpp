#include "core/game_rules.hpp"

namespace pg
{
	GameRules parseGameRules(JsonReader &p_reader)
	{
		p_reader.forbidUnknown({"version", "maxVerticalTraversalGap", "overlayMasks"});
		if (p_reader.require<int>("version") != 1)
		{
			throw JsonError(p_reader.file(), p_reader.pathFor("version"), "unsupported version");
		}

		JsonReader overlayMasksReader = p_reader.child("overlayMasks");
		overlayMasksReader.forbidUnknown({"hovered", "invalid"});
		GameRules result;
		result.maxVerticalTraversalGap = p_reader.require<double>("maxVerticalTraversalGap");
		result.overlayMasks.emplace("hovered", overlayMasksReader.require<std::array<int, 2>>("hovered"));
		result.overlayMasks.emplace("invalid", overlayMasksReader.require<std::array<int, 2>>("invalid"));
		return result;
	}
}
