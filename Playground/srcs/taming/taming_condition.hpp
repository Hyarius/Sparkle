#pragma once

#include "feats/battle_condition.hpp"

#include <memory>

namespace pg
{
	class JsonReader;
	using TamingCondition = BattleCondition;

	[[nodiscard]] std::unique_ptr<TamingCondition> parseTamingCondition(JsonReader &p_reader);
}
