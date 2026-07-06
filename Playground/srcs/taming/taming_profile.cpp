#include "taming/taming_profile.hpp"

#include "core/json.hpp"

namespace pg
{
	bool TamingProfile::empty() const noexcept
	{
		return conditions.empty();
	}

	TamingProfile parseTamingProfile(JsonReader &p_reader)
	{
		p_reader.forbidUnknown({"conditions"});
		TamingProfile result;
		for (JsonReader conditionReader : p_reader.childArray("conditions"))
		{
			result.conditions.push_back(parseTamingCondition(conditionReader));
		}
		return result;
	}
}
