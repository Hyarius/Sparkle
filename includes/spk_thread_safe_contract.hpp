#pragma once

#include <memory>
#include <mutex>
#include <utility>

#include "spk_blockable_trait.hpp"

namespace spk
{
	template <typename TContract, typename TMutex = std::mutex>
	class ThreadSafeContract
	{
	private:
		using NativeBlocker = typename TContract::Blocker;

		TContract _contract;
		std::shared_ptr<TMutex> _mutex;

	public:
		class Blocker
		{
		private:
			NativeBlocker _blocker;
			std::shared_ptr<TMutex> _mutex;

		public:
			Blocker() = default;

			Blocker(NativeBlocker p_blocker, std::shared_ptr<TMutex> p_mutex) :
				_blocker(std::move(p_blocker)),
				_mutex(std::move(p_mutex))
			{
			}

			~Blocker()
			{
				release();
			}

			Blocker(const Blocker&) = delete;
			Blocker& operator=(const Blocker&) = delete;

			Blocker(Blocker&& p_other) noexcept :
				_blocker(std::move(p_other._blocker)),
				_mutex(std::move(p_other._mutex))
			{
			}

			Blocker& operator=(Blocker&& p_other) noexcept
			{
				if (this != &p_other)
				{
					release();

					_blocker = std::move(p_other._blocker);
					_mutex = std::move(p_other._mutex);
				}

				return *this;
			}

			void release()
			{
				std::shared_ptr<TMutex> mutex = _mutex;

				if (mutex == nullptr)
				{
					return;
				}

				std::scoped_lock lock(*mutex);
				_blocker.release();
			}

			[[nodiscard]] bool isValid() const
			{
				std::shared_ptr<TMutex> mutex = _mutex;

				if (mutex == nullptr)
				{
					return false;
				}

				std::scoped_lock lock(*mutex);
				return _blocker.isValid();
			}
		};

		ThreadSafeContract() = default;

		ThreadSafeContract(TContract p_contract, std::shared_ptr<TMutex> p_mutex) :
			_contract(std::move(p_contract)),
			_mutex(std::move(p_mutex))
		{
		}

		~ThreadSafeContract()
		{
			resign();
		}

		ThreadSafeContract(const ThreadSafeContract&) = delete;
		ThreadSafeContract& operator=(const ThreadSafeContract&) = delete;

		ThreadSafeContract(ThreadSafeContract&& p_other) noexcept :
			_contract(std::move(p_other._contract)),
			_mutex(std::move(p_other._mutex))
		{
		}

		ThreadSafeContract& operator=(ThreadSafeContract&& p_other) noexcept
		{
			if (this != &p_other)
			{
				resign();

				_contract = std::move(p_other._contract);
				_mutex = std::move(p_other._mutex);
			}

			return *this;
		}

		void resign()
		{
			std::shared_ptr<TMutex> mutex = _mutex;

			if (mutex == nullptr)
			{
				return;
			}

			std::scoped_lock lock(*mutex);
			_contract.resign();
		}

		void relinquish()
		{
			std::shared_ptr<TMutex> mutex = _mutex;

			if (mutex == nullptr)
			{
				return;
			}

			{
				std::scoped_lock lock(*mutex);
				_contract.relinquish();
			}

			_mutex = nullptr;
		}

		[[nodiscard]] Blocker block(BlockableTrait::Mode p_mode = BlockableTrait::Mode::Ignore)
		{
			std::shared_ptr<TMutex> mutex = _mutex;

			if (mutex == nullptr)
			{
				return Blocker();
			}

			std::scoped_lock lock(*mutex);
			return Blocker(_contract.block(p_mode), mutex);
		}

		[[nodiscard]] bool isValid() const
		{
			std::shared_ptr<TMutex> mutex = _mutex;

			if (mutex == nullptr)
			{
				return false;
			}

			std::scoped_lock lock(*mutex);
			return _contract.isValid();
		}
	};
}
