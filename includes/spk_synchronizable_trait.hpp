#pragma once

namespace spk
{
	class SynchronizableTrait
	{
	private:
		bool _needsSynchronization = false;

	protected:
		SynchronizableTrait() = default;
		~SynchronizableTrait() = default;

		virtual void _synchronize() = 0;

	public:
		SynchronizableTrait(const SynchronizableTrait&) = delete;
		SynchronizableTrait& operator=(const SynchronizableTrait&) = delete;

		SynchronizableTrait(SynchronizableTrait&&) noexcept = delete;
		SynchronizableTrait& operator=(SynchronizableTrait&&) noexcept = delete;

		void requestSynchronization() noexcept
		{
			_needsSynchronization = true;
		}

		bool needsSynchronization() const noexcept
		{
			return _needsSynchronization;
		}

		void synchronize()
		{
			if (_needsSynchronization == false)
			{
				return;
			}

			_synchronize();
			_needsSynchronization = false;
		}

		void forceSynchronization()
		{
			_synchronize();
			_needsSynchronization = false;
		}
	};
}