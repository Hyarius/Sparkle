#pragma once

#include "conditions/condition_definition.hpp"
#include "core/json.hpp"

#include <vector>

namespace pg
{
	// What a wild creature has to be shown before it lets itself be caught. It is not a registry
	// definition and has no file or id of its own: step 04 embeds it in a species, where an
	// absent profile means untameable.
	//
	// It shares the Feat condition parser and the same closed variant, because the GDD asks the
	// same structured question of both domains. Only the binding differs: here the subject is
	// the tracked wild unit, so "subjectTeam" is the Enemy side and "opponentTeam" is the
	// player's - which is why condition JSON never names an absolute side.
	//
	// Every advancement is encounter-local. Even a "game" window is reset when the encounter is
	// constructed, and partial taming progress is never saved. Step 16 binds the perspective and
	// performs the impression; nothing here evaluates anything.
	struct TamingProfileDefinition
	{
		// ANDed: all of them must complete. anyOf supplies explicit alternatives.
		std::vector<ConditionSpec> conditions;
	};

	// A present profile means the species is tameable, so an empty condition list is an
	// authoring error rather than "always tameable".
	[[nodiscard]] TamingProfileDefinition parseTamingProfile(JsonReader &p_reader, const ConditionLimits &p_limits);
}
