#pragma once

#include <algorithm>
#include <functional>
#include <limits>
#include <memory>
#include <mutex>
#include <stdexcept>
#include <utility>
#include <vector>

namespace spk
{
	template <typename TType>
	class ObjectPool
	{
	private:
		struct Data
		{
			mutable std::mutex mutex;
			const std::move_only_function<std::unique_ptr<TType>()> generator;
			const std::move_only_function<void(TType &)> onObtain;
			std::vector<std::unique_ptr<TType>> availableObjects;
			std::size_t maximumCachedObjectCount = std::numeric_limits<std::size_t>::max();
			bool closed = false;

			Data(std::move_only_function<std::unique_ptr<TType>()> p_generator, std::move_only_function<void(TType &)> p_onObtain) :
				generator(std::move(p_generator)),
				onObtain(std::move(p_onObtain))
			{
			}
		};

	public:
		using Generator = std::move_only_function<std::unique_ptr<TType>()>;
		using OnObtain = std::move_only_function<void(TType &)>;

		class ReturnToPoolDeleter
		{
		private:
			std::weak_ptr<Data> _data;

		public:
			ReturnToPoolDeleter() = default;
			explicit ReturnToPoolDeleter(std::weak_ptr<Data> p_data) noexcept :
				_data(std::move(p_data))
			{
			}

			void operator()(TType *p_ptr) const noexcept
			{
				std::unique_ptr<TType> object(p_ptr);
				if (!object)
				{
					return;
				}
				const std::shared_ptr<Data> data = _data.lock();
				if (!data)
				{
					return;
				}
				try
				{
					std::scoped_lock lock(data->mutex);
					if (data->closed || data->availableObjects.size() >= data->maximumCachedObjectCount)
					{
						return;
					}
					data->availableObjects.emplace_back(std::move(object));
				} catch (...)
				{
					// The local unique_ptr destroys the object when caching fails.
				}
			}
		};

		using Handle = std::unique_ptr<TType, ReturnToPoolDeleter>;

	private:
		std::shared_ptr<Data> _data;

		void _throwIfClosed(const Data &p_data) const
		{
			if (p_data.closed)
			{
				throw std::runtime_error("spk::ObjectPool: pool is closed");
			}
		}

	public:
		explicit ObjectPool(Generator p_generator, OnObtain p_onObtain = nullptr) :
			_data(std::make_shared<Data>(std::move(p_generator), std::move(p_onObtain)))
		{
			if (!_data->generator)
			{
				throw std::logic_error("spk::ObjectPool: generator not set");
			}
		}

		~ObjectPool()
		{
			close();
		}
		ObjectPool(const ObjectPool &) = delete;
		ObjectPool &operator=(const ObjectPool &) = delete;
		ObjectPool(ObjectPool &&) = delete;
		ObjectPool &operator=(ObjectPool &&) = delete;

		[[nodiscard]] Handle acquire()
		{
			std::unique_ptr<TType> object;
			{
				std::scoped_lock lock(_data->mutex);
				_throwIfClosed(*_data);
				if (!_data->availableObjects.empty())
				{
					object = std::move(_data->availableObjects.back());
					_data->availableObjects.pop_back();
				}
			}
			if (!object)
			{
				object = _data->generator();
				if (!object)
				{
					throw std::logic_error("spk::ObjectPool: generator returned nullptr");
				}
			}
			if (_data->onObtain)
			{
				_data->onObtain(*object);
			}
			return Handle(object.release(), ReturnToPoolDeleter(_data));
		}

		void preallocate(std::size_t p_count)
		{
			std::size_t missing = 0;
			{
				std::scoped_lock lock(_data->mutex);
				_throwIfClosed(*_data);
				const std::size_t target = std::min(p_count, _data->maximumCachedObjectCount);
				missing = target > _data->availableObjects.size() ? target - _data->availableObjects.size() : 0;
			}
			std::vector<std::unique_ptr<TType>> created;
			created.reserve(missing);
			for (std::size_t index = 0; index < missing; ++index)
			{
				std::unique_ptr<TType> object = _data->generator();
				if (!object)
				{
					throw std::logic_error("spk::ObjectPool: generator returned nullptr");
				}
				created.emplace_back(std::move(object));
			}
			std::scoped_lock lock(_data->mutex);
			_throwIfClosed(*_data);
			while (!created.empty() && _data->availableObjects.size() < _data->maximumCachedObjectCount)
			{
				_data->availableObjects.emplace_back(std::move(created.back()));
				created.pop_back();
			}
		}

		void purge()
		{
			std::scoped_lock lock(_data->mutex);
			_throwIfClosed(*_data);
			_data->availableObjects.clear();
		}

		void setMaximumCachedObjectCount(std::size_t p_count)
		{
			std::scoped_lock lock(_data->mutex);
			_throwIfClosed(*_data);
			_data->maximumCachedObjectCount = p_count;
			while (_data->availableObjects.size() > p_count)
			{
				_data->availableObjects.pop_back();
			}
		}

		[[nodiscard]] std::size_t nbAvailableObject() const noexcept
		{
			std::scoped_lock lock(_data->mutex);
			return _data->availableObjects.size();
		}

		[[nodiscard]] bool empty() const noexcept
		{
			return nbAvailableObject() == 0;
		}
		[[nodiscard]] bool isClosed() const noexcept
		{
			std::scoped_lock lock(_data->mutex);
			return _data->closed;
		}

		void close() noexcept
		{
			if (!_data)
			{
				return;
			}
			std::scoped_lock lock(_data->mutex);
			if (_data->closed)
			{
				return;
			}
			_data->closed = true;
			_data->availableObjects.clear();
		}
	};
}
