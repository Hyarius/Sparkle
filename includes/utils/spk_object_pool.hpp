#pragma once

#include <functional>
#include <limits>
#include <memory>
#include <stdexcept>
#include <utility>
#include <vector>

namespace spk
{
	template <typename TType>
	class ObjectPool
	{
	private:
		struct Data;

	public:
		using Generator = std::function<std::unique_ptr<TType>()>;
		using Cleaner = std::function<void(TType&)>;
		using RealDestructor = std::function<void(TType&)>;

		class ReturnToPoolDeleter
		{
		private:
			std::weak_ptr<Data> _data;

		public:
			ReturnToPoolDeleter() = default;

			explicit ReturnToPoolDeleter(const std::weak_ptr<Data>& p_data) :
				_data(p_data)
			{
			}

			void operator()(TType* p_ptr) const
			{
				if (p_ptr == nullptr)
				{
					return;
				}

				std::shared_ptr<Data> data = _data.lock();
				if (data == nullptr || data->closed == true)
				{
					data = nullptr;
					delete p_ptr;
					return;
				}

				if (data->availableObjects.size() >= data->maximumCachedObjectCount)
				{
					if (data->realDestructor != nullptr)
					{
						data->realDestructor(*p_ptr);
					}

					delete p_ptr;
					return;
				}

				data->availableObjects.emplace_back(p_ptr);
			}
		};

		using Handle = std::unique_ptr<TType, ReturnToPoolDeleter>;

	private:
		struct Data
		{
			Generator generator = nullptr;
			Cleaner cleaner = nullptr;
			RealDestructor realDestructor = nullptr;

			std::vector<std::unique_ptr<TType>> availableObjects;
			size_t maximumCachedObjectCount = std::numeric_limits<size_t>::max();

			bool closed = false;
		};

		std::shared_ptr<Data> _data;

		TType* _obtainRawPointer()
		{
			if (_data->closed == true)
			{
				throw std::runtime_error("Can't obtain an object from a closed ObjectPool");
			}

			if (_data->generator == nullptr)
			{
				throw std::logic_error("spk::ObjectPool: generator not set");
			}

			TType* result = nullptr;

			if (_data->availableObjects.empty() == true)
			{
				std::unique_ptr<TType> createdObject = _data->generator();
				if (createdObject == nullptr)
				{
					throw std::logic_error("spk::ObjectPool: generator returned nullptr");
				}

				result = createdObject.release();
			}
			else
			{
				result = _data->availableObjects.back().release();
				_data->availableObjects.pop_back();
			}

			if (_data->cleaner != nullptr)
			{
				_data->cleaner(*result);
			}

			return result;
		}

	public:
		ObjectPool() :
			_data(std::make_shared<Data>())
		{
		}

		explicit ObjectPool(Generator p_generator, Cleaner p_cleaner = nullptr, RealDestructor p_realDestructor = nullptr) :
			_data(std::make_shared<Data>())
		{
			configure(std::move(p_generator), std::move(p_cleaner), std::move(p_realDestructor));
		}

		~ObjectPool()
		{
			close();
		}

		ObjectPool(const ObjectPool&) = delete;
		ObjectPool& operator=(const ObjectPool&) = delete;

		ObjectPool(ObjectPool&&) = delete;
		ObjectPool& operator=(ObjectPool&&) = delete;

		void configure(Generator p_generator, Cleaner p_cleaner = nullptr, RealDestructor p_realDestructor = nullptr)
		{
			if (_data->closed == true)
			{
				throw std::runtime_error("Can't configure a closed ObjectPool");
			}

			release();

			_data->generator = std::move(p_generator);
			_data->cleaner = std::move(p_cleaner);
			_data->realDestructor = std::move(p_realDestructor);
		}

		void setMaximumCachedObjectCount(size_t p_count)
		{
			if (_data->closed == true)
			{
				throw std::runtime_error("Can't edit maximum cached object count of a closed ObjectPool");
			}

			_data->maximumCachedObjectCount = p_count;

			while (_data->availableObjects.size() > _data->maximumCachedObjectCount)
			{
				if (_data->realDestructor != nullptr)
				{
					_data->realDestructor(*_data->availableObjects.back());
				}

				_data->availableObjects.pop_back();
			}
		}

		[[nodiscard]] size_t size() const noexcept
		{
			return _data->availableObjects.size();
		}

		[[nodiscard]] bool empty() const noexcept
		{
			return _data->availableObjects.empty();
		}

		[[nodiscard]] bool isClosed() const noexcept
		{
			return _data->closed;
		}

		[[nodiscard]] bool hasGenerator() const noexcept
		{
			return (_data->generator != nullptr);
		}

		[[nodiscard]] bool hasCleaner() const noexcept
		{
			return (_data->cleaner != nullptr);
		}

		void reserve(size_t p_count)
		{
			if (_data->closed == true)
			{
				throw std::runtime_error("Can't reserve objects in a closed ObjectPool");
			}

			if (_data->generator == nullptr)
			{
				throw std::logic_error("spk::ObjectPool: generator not set");
			}

			while (_data->availableObjects.size() < p_count)
			{
				std::unique_ptr<TType> createdObject = _data->generator();
				if (createdObject == nullptr)
				{
					throw std::logic_error("spk::ObjectPool: generator returned nullptr");
				}

				_data->availableObjects.emplace_back(std::move(createdObject));
			}
		}

		Handle obtain()
		{
			return Handle(_obtainRawPointer(), ReturnToPoolDeleter(_data));
		}

		void release()
		{
			if (_data->closed == true)
			{
				throw std::runtime_error("Can't release a closed ObjectPool");
			}

			if (_data->realDestructor != nullptr)
			{
				for (std::unique_ptr<TType>& element : _data->availableObjects)
				{
					if (element != nullptr)
					{
						_data->realDestructor(*element);
					}
				}
			}

			_data->availableObjects.clear();
		}

		void close() noexcept
		{
			if (_data == nullptr || _data->closed == true)
			{
				return;
			}

			_data->closed = true;
			_data->availableObjects.clear();
		}
	};
}