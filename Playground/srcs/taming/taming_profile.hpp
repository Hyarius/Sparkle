#pragma once

#include "taming/taming_condition.hpp"

#include <memory>
#include <vector>

namespace pg
{
	class JsonReader;

	struct TamingProfile
	{
		std::vector<std::unique_ptr<TamingCondition>> conditions;

		[[nodiscard]] bool empty() const noexcept;
	};

	[[nodiscard]] TamingProfile parseTamingProfile(JsonReader &p_reader);
}
