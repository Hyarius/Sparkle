#include "taming/taming_condition.hpp"

#include "feats/feat_requirement.hpp"

namespace pg
{
	std::unique_ptr<TamingCondition> parseTamingCondition(JsonReader &p_reader)
	{
		return parseBattleCondition(p_reader, BattleConditionCatalog::Taming);
	}
}
