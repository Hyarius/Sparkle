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

		void requestSynchronization() noexcept;
		bool needsSynchronization() const noexcept;
		void synchronize();
		void forceSynchronization();
	};
}