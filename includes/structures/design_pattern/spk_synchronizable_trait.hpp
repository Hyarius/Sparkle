#pragma once

#include <atomic>

namespace spk
{
	class SynchronizableTrait
	{
	private:
		mutable std::atomic_bool _needsSynchronization = false;

	protected:
		SynchronizableTrait() = default;
		~SynchronizableTrait() = default;

		virtual void _synchronize() const = 0;
		[[nodiscard]] bool _beginSynchronization() const noexcept;
		void _failSynchronization() const noexcept;

	public:
		SynchronizableTrait(const SynchronizableTrait &) = delete;
		SynchronizableTrait &operator=(const SynchronizableTrait &) = delete;

		SynchronizableTrait(SynchronizableTrait &&p_other) noexcept
			:
			_needsSynchronization(p_other._needsSynchronization.exchange(false))
		{
		}

		SynchronizableTrait &operator=(SynchronizableTrait &&p_other) noexcept
		{
			if (this != &p_other)
			{
				_needsSynchronization.store(p_other._needsSynchronization.exchange(false));
			}
			return *this;
		}

		void requestSynchronization() const noexcept;
		bool needsSynchronization() const noexcept;
		void synchronize() const;
		void forceSynchronization() const;
	};
}
