#pragma once

#include "battle/battle_time.hpp"

#include <cstdint>
#include <type_traits>
#include <variant>

namespace pg
{
	struct TimelineDurationState
	{
		BattleTime expiresAt;
	};
	struct OwnerActivationDurationState
	{
		std::uint32_t remainingActivations = 0;
	};
	struct InfiniteDurationState
	{
	};
	using DurationState = std::variant<TimelineDurationState, OwnerActivationDurationState, InfiniteDurationState>;

	struct TimelineDurationSnapshot
	{
		BattleTime expiresAt;
		[[nodiscard]] bool operator==(const TimelineDurationSnapshot &) const = default;
	};
	struct OwnerActivationDurationSnapshot
	{
		std::uint32_t remainingActivations = 0;
		[[nodiscard]] bool operator==(const OwnerActivationDurationSnapshot &) const = default;
	};
	struct InfiniteDurationSnapshot
	{
		[[nodiscard]] bool operator==(const InfiniteDurationSnapshot &) const = default;
	};
	using DurationSnapshot = std::variant<TimelineDurationSnapshot, OwnerActivationDurationSnapshot, InfiniteDurationSnapshot>;

	[[nodiscard]] inline DurationSnapshot snapshotDuration(const DurationState &p_duration)
	{
		return std::visit([](const auto &value) -> DurationSnapshot {
			using T = std::decay_t<decltype(value)>;
			if constexpr (std::is_same_v<T, TimelineDurationState>)
			{
				return TimelineDurationSnapshot{value.expiresAt};
			}
			else if constexpr (std::is_same_v<T, OwnerActivationDurationState>)
			{
				return OwnerActivationDurationSnapshot{value.remainingActivations};
			}
			else
			{
				return InfiniteDurationSnapshot{};
			}
		},
						  p_duration);
	}
}
