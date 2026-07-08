#pragma once

#include <cstddef>
#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

#include "structures/system/time/spk_chronometer.hpp"
#include "structures/system/time/spk_duration.hpp"

namespace spk
{
	class Profiler
	{
	public:
		class Probe
		{
		public:
			class Sample
			{
			private:
				Probe *_probe;
				spk::Chronometer _chronometer;

			public:
				explicit Sample(Probe &p_probe);
				~Sample();

				Sample(const Sample &) = delete;
				Sample &operator=(const Sample &) = delete;
				Sample(Sample &&) = delete;
				Sample &operator=(Sample &&) = delete;
			};

		private:
			mutable std::mutex _mutex;
			std::size_t _count = 0;
			spk::Duration _total;
			spk::Duration _last;
			spk::Duration _minimum;
			spk::Duration _maximum;

		public:
			Probe() = default;

			void record(const spk::Duration &p_duration);
			void reset();

			[[nodiscard]] std::size_t count() const;
			[[nodiscard]] spk::Duration last() const;
			[[nodiscard]] spk::Duration minimum() const;
			[[nodiscard]] spk::Duration maximum() const;
			[[nodiscard]] spk::Duration average() const;
			[[nodiscard]] spk::Duration total() const;
		};

		struct Snapshot
		{
			std::string name;
			std::size_t count = 0;
			spk::Duration last;
			spk::Duration minimum;
			spk::Duration maximum;
			spk::Duration average;
		};

	private:
		mutable std::mutex _mutex;
		std::map<std::string, std::unique_ptr<Probe>> _probes;

	public:
		Profiler() = default;

		[[nodiscard]] Probe &probe(const std::string &p_name);

		[[nodiscard]] std::vector<Snapshot> snapshot() const;
		void reset();
	};
}
