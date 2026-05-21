#pragma once

#include <cstdint>

#include "time/spk_duration.hpp"
#include "time/spk_timestamp.hpp"

namespace spk
{
	namespace TimeUtils
	{
		[[nodiscard]] std::uint64_t nowNsMono();
		[[nodiscard]] Timestamp getTime();
		void sleepFor(const spk::Duration& p_duration);
	}
}