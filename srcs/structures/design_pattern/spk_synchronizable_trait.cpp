#include "structures/design_pattern/spk_synchronizable_trait.hpp"

#include <stdexcept>

namespace spk
{
	void SynchronizableTrait::requestSynchronization() const noexcept
	{
		_state.fetch_or(Requested, std::memory_order_release);
	}
	bool SynchronizableTrait::needsSynchronization() const noexcept
	{
		return (_state.load(std::memory_order_acquire) & Requested) != 0;
	}
	bool SynchronizableTrait::isSynchronizing() const noexcept
	{
		return (_state.load(std::memory_order_acquire) & Synchronizing) != 0;
	}

	bool SynchronizableTrait::_tryBeginSynchronization(bool p_force) const noexcept
	{
		std::uint8_t state = _state.load(std::memory_order_acquire);
		for (;;)
		{
			if ((state & Synchronizing) != 0 || (!p_force && (state & Requested) == 0))
			{
				return false;
			}
			const std::uint8_t desired = static_cast<std::uint8_t>((state | Synchronizing) & ~Requested);
			if (_state.compare_exchange_weak(state, desired, std::memory_order_acq_rel, std::memory_order_acquire))
			{
				return true;
			}
		}
	}
	bool SynchronizableTrait::_beginSynchronization() const noexcept
	{
		return _tryBeginSynchronization(false);
	}
	void SynchronizableTrait::_completeSynchronization() const noexcept
	{
		_state.fetch_and(static_cast<std::uint8_t>(~Synchronizing), std::memory_order_release);
	}
	void SynchronizableTrait::_failSynchronization() const noexcept
	{
		std::uint8_t state = _state.load(std::memory_order_acquire);
		for (;;)
		{
			const std::uint8_t desired = static_cast<std::uint8_t>((state | Requested) & ~Synchronizing);
			if (_state.compare_exchange_weak(state, desired, std::memory_order_release, std::memory_order_acquire))
			{
				return;
			}
		}
	}
	bool SynchronizableTrait::synchronize() const
	{
		if (!_tryBeginSynchronization(false))
		{
			return false;
		}
		try
		{
			_synchronize();
		} catch (...)
		{
			_failSynchronization();
			throw;
		}
		_completeSynchronization();
		return true;
	}
	void SynchronizableTrait::forceSynchronization() const
	{
		if (!_tryBeginSynchronization(true))
		{
			throw std::logic_error("spk::SynchronizableTrait: synchronization already in progress");
		}
		try
		{
			_synchronize();
		} catch (...)
		{
			_failSynchronization();
			throw;
		}
		_completeSynchronization();
	}
}
