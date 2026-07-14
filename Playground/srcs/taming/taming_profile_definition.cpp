#include "taming/taming_profile_definition.hpp"

namespace pg
{
	TamingProfileDefinition parseTamingProfile(JsonReader &p_reader, const ConditionLimits &p_limits)
	{
		p_reader.forbidUnknown({"conditions"});

		ConditionParseState state;
		state.limits = p_limits;

		TamingProfileDefinition result;
		result.conditions = parseConditions(p_reader, "conditions", state, false);
		return result;
	}
}
