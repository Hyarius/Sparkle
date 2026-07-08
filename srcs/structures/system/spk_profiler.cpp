#include "structures/system/spk_profiler.hpp"

#include "type/spk_time_unit.hpp"

namespace spk
{
	Profiler::Probe::Sample::Sample(Profiler::Probe &p_probe) :
		_probe(&p_probe)
	{
		_chronometer.start();
	}

	Profiler::Probe::Sample::~Sample()
	{
		_probe->record(_chronometer.elapsedTime());
	}

	void Profiler::Probe::record(const spk::Duration &p_duration)
	{
		const std::lock_guard<std::mutex> lock(_mutex);
		if (_count == 0 || p_duration < _minimum)
		{
			_minimum = p_duration;
		}
		if (_count == 0 || p_duration > _maximum)
		{
			_maximum = p_duration;
		}
		_last = p_duration;
		_total += p_duration;
		++_count;
	}

	void Profiler::Probe::reset()
	{
		const std::lock_guard<std::mutex> lock(_mutex);
		_count = 0;
		_total = spk::Duration();
		_last = spk::Duration();
		_minimum = spk::Duration();
		_maximum = spk::Duration();
	}

	std::size_t Profiler::Probe::count() const
	{
		const std::lock_guard<std::mutex> lock(_mutex);
		return _count;
	}

	spk::Duration Profiler::Probe::last() const
	{
		const std::lock_guard<std::mutex> lock(_mutex);
		return _last;
	}

	spk::Duration Profiler::Probe::minimum() const
	{
		const std::lock_guard<std::mutex> lock(_mutex);
		return _minimum;
	}

	spk::Duration Profiler::Probe::maximum() const
	{
		const std::lock_guard<std::mutex> lock(_mutex);
		return _maximum;
	}

	spk::Duration Profiler::Probe::average() const
	{
		const std::lock_guard<std::mutex> lock(_mutex);
		if (_count == 0)
		{
			return spk::Duration();
		}
		const long long averageNs = _total.nanoseconds() / static_cast<long long>(_count);
		return spk::Duration(static_cast<long double>(averageNs), spk::TimeUnit::Nanosecond);
	}

	spk::Duration Profiler::Probe::total() const
	{
		const std::lock_guard<std::mutex> lock(_mutex);
		return _total;
	}

	Profiler::Probe &Profiler::probe(const std::string &p_name)
	{
		const std::lock_guard<std::mutex> lock(_mutex);
		std::unique_ptr<Probe> &slot = _probes[p_name];
		if (slot == nullptr)
		{
			slot = std::make_unique<Probe>();
		}
		return *slot;
	}

	std::vector<Profiler::Snapshot> Profiler::snapshot() const
	{
		const std::lock_guard<std::mutex> lock(_mutex);
		std::vector<Snapshot> result;
		result.reserve(_probes.size());
		for (const auto &[name, probe] : _probes)
		{
			result.push_back(Snapshot{
				.name = name,
				.count = probe->count(),
				.last = probe->last(),
				.minimum = probe->minimum(),
				.maximum = probe->maximum(),
				.average = probe->average()});
		}
		return result;
	}

	void Profiler::reset()
	{
		const std::lock_guard<std::mutex> lock(_mutex);
		for (const auto &[name, probe] : _probes)
		{
			probe->reset();
		}
	}
}
