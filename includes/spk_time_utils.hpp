#pragma once

#include <cstdint>

#include "spk_duration.hpp"
#include "spk_timestamp.hpp"

namespace spk
{
	namespace TimeUtils
	{
		[[nodiscard]] std::uint64_t nowNsMono();
		[[nodiscard]] Timestamp getTime();
		void sleepFor(const spk::Duration& p_duration);
	}
}