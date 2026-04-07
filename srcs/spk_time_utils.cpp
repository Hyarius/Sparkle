#include "spk_time_utils.hpp"

#include <chrono>
#include <thread>

namespace spk
{
	namespace TimeUtils
	{
		std::uint64_t nowNsMono()
		{
			const auto now = std::chrono::steady_clock::now().time_since_epoch();
			return static_cast<std::uint64_t>(
				std::chrono::duration_cast<std::chrono::nanoseconds>(now).count());
		}

		Timestamp getTime()
		{
			return Timestamp(static_cast<long double>(nowNsMono()), TimeUnit::Nanosecond);
		}

		void sleepFor(const spk::Duration& p_duration)
		{
			if (p_duration < 1000_ns)
			{
				return;
			}

			long long startingSleepTimestamp = spk::TimeUtils::getTime().nanoseconds();
			long long currentTimeInNS = startingSleepTimestamp;
			long long sleepDuration = 0;
			long long finalFrameTimeStamp = startingSleepTimestamp + p_duration.nanoseconds();

			while (currentTimeInNS + sleepDuration <= finalFrameTimeStamp)
			{
				currentTimeInNS = spk::TimeUtils::getTime().nanoseconds();
				if (sleepDuration == 0)
				{
					sleepDuration = currentTimeInNS - startingSleepTimestamp;
				}
			}
		}
	}
}