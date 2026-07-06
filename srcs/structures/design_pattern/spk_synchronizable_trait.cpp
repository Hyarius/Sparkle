#include "structures/design_pattern/spk_synchronizable_trait.hpp"

namespace spk
{
	void SynchronizableTrait::requestSynchronization() const noexcept
	{
		_needsSynchronization.store(true);
	}

	bool SynchronizableTrait::needsSynchronization() const noexcept
	{
		return _needsSynchronization.load();
	}

	bool SynchronizableTrait::_beginSynchronization() const noexcept
	{
		return _needsSynchronization.exchange(false);
	}

	void SynchronizableTrait::_failSynchronization() const noexcept
	{
		_needsSynchronization.store(true);
	}

	void SynchronizableTrait::synchronize() const
	{
		if (_beginSynchronization() == false)
		{
			return;
		}

		try
		{
			_synchronize();
		} catch (...)
		{
			_failSynchronization();
			throw;
		}
	}

	void SynchronizableTrait::forceSynchronization() const
	{
		(void)_beginSynchronization();
		try
		{
			_synchronize();
		} catch (...)
		{
			_failSynchronization();
			throw;
		}
	}
}
