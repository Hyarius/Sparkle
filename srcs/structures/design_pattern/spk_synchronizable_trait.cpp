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

	void SynchronizableTrait::synchronize() const
	{
		if (_needsSynchronization.load() == false)
		{
			return;
		}

		if (_needsSynchronization.exchange(false) == false)
		{
			return;
		}

		try
		{
			_synchronize();
		} catch (...)
		{
			_needsSynchronization.store(true);
			throw;
		}
	}

	void SynchronizableTrait::forceSynchronization() const
	{
		_synchronize();
		_needsSynchronization.store(false);
	}
}
