#include "spk_chronometer.hpp"

#include "spk_time_utils.hpp"

namespace spk
{
	void Chronometer::start()
	{
		if (_state != State::Idle)
		{
			return;
		}

		_accumulatedTime = Duration();
		_startTime = TimeUtils::getTime();
		_state = State::Running;
	}

	void Chronometer::stop() noexcept
	{
		_state = State::Idle;
		_startTime = Timestamp();
		_accumulatedTime = Duration();
	}

	void Chronometer::pause()
	{
		if (_state != State::Running)
		{
			return;
		}

		_accumulatedTime += _currentRunDuration();
		_state = State::Paused;
	}

	void Chronometer::resume()
	{
		if (_state != State::Paused)
		{
			return;
		}

		_startTime = TimeUtils::getTime();
		_state = State::Running;
	}

	void Chronometer::restart()
	{
		_accumulatedTime = Duration();
		_startTime = TimeUtils::getTime();
		_state = State::Running;
	}

	Duration Chronometer::elapsedTime() const noexcept
	{
		if (_state == State::Running)
		{
			return _accumulatedTime + _currentRunDuration();
		}

		return _accumulatedTime;
	}

	Chronometer::State Chronometer::state() const noexcept
	{
		return _state;
	}

	Duration Chronometer::_currentRunDuration() const noexcept
	{
		if (_state != State::Running)
		{
			return Duration();
		}

		return TimeUtils::getTime() - _startTime;
	}

	const char* toString(Chronometer::State p_state) noexcept
	{
		switch (p_state)
		{
		case Chronometer::State::Idle:
			return "Idle";

		case Chronometer::State::Running:
			return "Running";

		case Chronometer::State::Paused:
			return "Paused";
		}

		return "UnknownState";
	}

	const wchar_t* toWstring(Chronometer::State p_state) noexcept
	{
		switch (p_state)
		{
		case Chronometer::State::Idle:
			return L"Idle";

		case Chronometer::State::Running:
			return L"Running";

		case Chronometer::State::Paused:
			return L"Paused";
		}

		return L"UnknownState";
	}
}

std::ostream& operator<<(std::ostream& p_os, spk::Chronometer::State p_state)
{
	return (p_os << spk::toString(p_state));
}

std::wostream& operator<<(std::wostream& p_wos, spk::Chronometer::State p_state)
{
	return (p_wos << spk::toWstring(p_state));
}