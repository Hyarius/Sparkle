#pragma once

#include <atomic>
#include <cstdint>

namespace spk
{
	class SynchronizableTrait
	{
	private:
		enum StateFlag : std::uint8_t
		{
			None = 0,
			Requested = 1 << 0,
			Synchronizing = 1 << 1
		};
		mutable std::atomic<std::uint8_t> _state = None;

	protected:
		SynchronizableTrait() = default;
		~SynchronizableTrait() = default;
		virtual void _synchronize() const = 0;
		// For asynchronous implementations that split synchronization work across
		// threads, these preserve the same state transition as synchronize().
		[[nodiscard]] bool _beginSynchronization() const noexcept;
		void _completeSynchronization() const noexcept;
		void _failSynchronization() const noexcept;

	private:
		[[nodiscard]] bool _tryBeginSynchronization(bool p_force) const noexcept;

	public:
		SynchronizableTrait(const SynchronizableTrait &) = delete;
		SynchronizableTrait &operator=(const SynchronizableTrait &) = delete;
		SynchronizableTrait(SynchronizableTrait &&) = delete;
		SynchronizableTrait &operator=(SynchronizableTrait &&) = delete;
		void requestSynchronization() const noexcept;
		[[nodiscard]] bool needsSynchronization() const noexcept;
		[[nodiscard]] bool isSynchronizing() const noexcept;
		[[nodiscard]] bool synchronize() const;
		void forceSynchronization() const;
	};
}
