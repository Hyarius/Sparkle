#pragma once

#include <iosfwd>

#include "spk_duration.hpp"
#include "spk_timestamp.hpp"

namespace spk
{
	class Chronometer
	{
	public:
		enum class State
		{
			Idle,
			Running,
			Paused
		};

	private:
		State _state = State::Idle;
		Timestamp _startTime;
		Duration _accumulatedTime;

		[[nodiscard]] Duration _currentRunDuration() const noexcept;

	public:
		Chronometer() = default;
		~Chronometer() = default;

		Chronometer(const Chronometer& p_other) = default;
		Chronometer(Chronometer&& p_other) noexcept = default;

		Chronometer& operator=(const Chronometer& p_other) = default;
		Chronometer& operator=(Chronometer&& p_other) noexcept = default;

		void start();
		void stop() noexcept;
		void pause();
		void resume();
		void restart();

		[[nodiscard]] Duration elapsedTime() const noexcept;
		[[nodiscard]] State state() const noexcept;
	};

	[[nodiscard]] const char* toString(Chronometer::State p_state) noexcept;
	[[nodiscard]] const wchar_t* toWstring(Chronometer::State p_state) noexcept;
}

std::ostream& operator<<(std::ostream& p_os, spk::Chronometer::State p_state);
std::wostream& operator<<(std::wostream& p_wos, spk::Chronometer::State p_state);