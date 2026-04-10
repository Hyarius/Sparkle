#include "spk_timer.hpp"

#include <algorithm>

namespace spk
{
	Timer::Timer(const Duration& p_expectedDuration) :
		_expectedDuration(p_expectedDuration)
	{
	}

	void Timer::_synchronizeTimedOutState() const noexcept
	{
		if (_state != State::Running)
		{
			return;
		}

		if (_chronometer.elapsedTime() >= _expectedDuration)
		{
			_chronometer.pause();
			_state = State::TimedOut;
		}
	}

	Timer::State Timer::state() const noexcept
	{
		_synchronizeTimedOutState();
		return _state;
	}

	Duration Timer::elapsed() const noexcept
	{
		_synchronizeTimedOutState();
		return _chronometer.elapsedTime();
	}

	Duration Timer::expectedDuration() const noexcept
	{
		return _expectedDuration;
	}

	float Timer::elapsedRatio() const noexcept
	{
		const long long expectedNanoseconds = _expectedDuration.nanoseconds();

		if (expectedNanoseconds <= 0LL)
		{
			return 0.0f;
		}

		const long double elapsedNanoseconds = static_cast<long double>(elapsed().nanoseconds());
		const long double expectedNanosecondsAsLongDouble = static_cast<long double>(expectedNanoseconds);
		const long double ratio = elapsedNanoseconds / expectedNanosecondsAsLongDouble;

		return static_cast<float>(std::clamp(ratio, 0.0L, 1.0L));
	}

	bool Timer::hasTimedOut() const noexcept
	{
		_synchronizeTimedOutState();
		return (_state == State::TimedOut);
	}

	void Timer::start()
	{
		_chronometer.restart();
		_state = State::Running;
		_synchronizeTimedOutState();
	}

	void Timer::stop() noexcept
	{
		_chronometer.stop();
		_state = State::Idle;
	}

	void Timer::pause()
	{
		_synchronizeTimedOutState();

		if (_state != State::Running)
		{
			return;
		}

		_chronometer.pause();
		_state = State::Paused;
	}

	void Timer::resume()
	{
		if (_state != State::Paused)
		{
			return;
		}

		_chronometer.resume();
		_state = State::Running;
		_synchronizeTimedOutState();
	}

	const char* toString(Timer::State p_state) noexcept
	{
		switch (p_state)
		{
		case Timer::State::Idle:
			return "Idle";

		case Timer::State::Running:
			return "Running";

		case Timer::State::Paused:
			return "Paused";

		case Timer::State::TimedOut:
			return "TimedOut";
		}

		return "UnknownState";
	}

	const wchar_t* toWstring(Timer::State p_state) noexcept
	{
		switch (p_state)
		{
		case Timer::State::Idle:
			return L"Idle";

		case Timer::State::Running:
			return L"Running";

		case Timer::State::Paused:
			return L"Paused";

		case Timer::State::TimedOut:
			return L"TimedOut";
		}

		return L"UnknownState";
	}
}

std::ostream& operator<<(std::ostream& p_os, spk::Timer::State p_state)
{
	return (p_os << spk::toString(p_state));
}

std::wostream& operator<<(std::wostream& p_wos, spk::Timer::State p_state)
{
	return (p_wos << spk::toWstring(p_state));
}