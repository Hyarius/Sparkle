#include "spk_synchronizable_trait.hpp"

namespace spk
{
		void SynchronizableTrait::requestSynchronization() noexcept
		{
			_needsSynchronization = true;
		}

		bool SynchronizableTrait::needsSynchronization() const noexcept
		{
			return _needsSynchronization;
		}

		void SynchronizableTrait::synchronize()
		{
			if (_needsSynchronization == false)
			{
				return;
			}

			_synchronize();
			_needsSynchronization = false;
		}

		void SynchronizableTrait::forceSynchronization()
		{
			_synchronize();
			_needsSynchronization = false;
		}
}