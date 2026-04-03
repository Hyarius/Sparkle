#pragma once

#include <cstddef>
#include <memory>

namespace spk
{
	

	class BlockableTrait
	{
	private:
		std::shared_ptr<size_t> _blockCounter = std::make_shared<size_t>(0);

	protected:
		BlockableTrait() = default;
		~BlockableTrait() = default;

	public:
		class Blocker
		{
		private:
			std::weak_ptr<size_t> _counter;

		public:
			Blocker() = default;

			explicit Blocker(const std::shared_ptr<size_t>& p_counter) :
				_counter(p_counter)
			{
				std::shared_ptr<size_t> counter = _counter.lock();
				if (counter != nullptr)
				{
					++(*counter);
				}
			}

			~Blocker()
			{
				release();
			}

			Blocker(const Blocker&) = delete;
			Blocker& operator=(const Blocker&) = delete;

			Blocker(Blocker&& p_other) noexcept :
				_counter(std::move(p_other._counter))
			{
			}

			Blocker& operator=(Blocker&& p_other) noexcept
			{
				if (this != &p_other)
				{
					release();
					_counter = std::move(p_other._counter);
				}

				return (*this);
			}

			void release()
			{
				std::shared_ptr<size_t> counter = _counter.lock();
				if (counter != nullptr && *counter > 0)
				{
					--(*counter);
				}

				_counter.reset();
			}

			bool isValid() const
			{
				return (_counter.expired() == false);
			}
		};

		Blocker block()
		{
			return Blocker(_blockCounter);
		}

		bool isBlocked() const
		{
			return (_blockCounter != nullptr && *_blockCounter > 0);
		}
	};
}