#include "traits/spk_synchronizable_trait.hpp"

namespace spk
{
		void SynchronizableTrait::requestSynchronization() noexcept
		{
			_needsSynchronization.store(true);
		}

		bool SynchronizableTrait::needsSynchronization() const noexcept
		{
			return _needsSynchronization.load();
		}

		void SynchronizableTrait::synchronize()
		{
			if (_needsSynchronization.exchange(false) == false)
			{
				return;
			}

			try
			{
				_synchronize();
			}
			catch (...)
			{
				_needsSynchronization.store(true);
				throw;
			}
		}

		void SynchronizableTrait::forceSynchronization()
		{
			_synchronize();
			_needsSynchronization.store(false);
		}
}
