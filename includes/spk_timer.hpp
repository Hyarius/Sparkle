#pragma once

#include <iosfwd>

#include "spk_chronometer.hpp"
#include "spk_duration.hpp"

namespace spk
{
	class Timer
	{
	public:
		enum class State
		{
			Idle,
			Running,
			Paused,
			TimedOut
		};

	private:
		mutable State _state = State::Idle;
		Duration _expectedDuration;
		mutable Chronometer _chronometer;

		void _synchronizeTimedOutState() const noexcept;

	public:
		Timer() = default;
		explicit Timer(const Duration& p_expectedDuration);

		Timer(const Timer& p_other) = default;
		Timer(Timer&& p_other) noexcept = default;

		Timer& operator=(const Timer& p_other) = default;
		Timer& operator=(Timer&& p_other) noexcept = default;

		[[nodiscard]] State state() const noexcept;
		[[nodiscard]] Duration elapsed() const noexcept;
		[[nodiscard]] Duration expectedDuration() const noexcept;
		[[nodiscard]] float elapsedRatio() const noexcept;
		[[nodiscard]] bool hasTimedOut() const noexcept;

		void start();
		void stop() noexcept;
		void pause();
		void resume();
	};

	[[nodiscard]] const char* toString(Timer::State p_state) noexcept;
	[[nodiscard]] const wchar_t* toWstring(Timer::State p_state) noexcept;
}

std::ostream& operator<<(std::ostream& p_os, spk::Timer::State p_state);
std::wostream& operator<<(std::wostream& p_wos, spk::Timer::State p_state);