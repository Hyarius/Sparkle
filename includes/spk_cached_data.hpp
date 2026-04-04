#pragma once

#include <concepts>
#include <functional>
#include <mutex>
#include <optional>
#include <stdexcept>
#include <utility>

namespace spk
{
	template <typename TType>
	requires (!std::is_void_v<TType>)
	class CachedData
	{
	public:
		using ValueType = TType;
		using Generator = std::function<TType()>;
		using Destructor = std::function<void(TType&)>;

	private:
		Generator _generator = nullptr;
		Destructor _destructor = nullptr;

		mutable std::optional<TType> _data;
		mutable std::mutex _mutex;

		void _destroyDataLocked() const
		{
			if (_data.has_value() == false)
			{
				return;
			}

			if (_destructor != nullptr)
			{
				_destructor(*_data);
			}

			_data.reset();
		}

		void _generateDataLocked() const
		{
			if (_data.has_value() == true)
			{
				return;
			}

			if (_generator == nullptr)
			{
				throw std::logic_error("spk::CachedData: generator not set");
			}

			_data = _generator();
		}

	public:
		CachedData() = default;

		explicit CachedData(Generator p_generator, Destructor p_destructor = nullptr) :
			_generator(std::move(p_generator)),
			_destructor(std::move(p_destructor))
		{
		}

		~CachedData()
		{
			release();
		}

		CachedData(const CachedData&) = delete;
		CachedData& operator=(const CachedData&) = delete;

		CachedData(CachedData&&) = delete;
		CachedData& operator=(CachedData&&) = delete;

		TType& get()
		{
			std::scoped_lock lock(_mutex);
			_generateDataLocked();
			return _data.value();
		}

		const TType& get() const
		{
			std::scoped_lock lock(_mutex);
			_generateDataLocked();
			return _data.value();
		}

		TType& operator*()
		{
			return get();
		}

		const TType& operator*() const
		{
			return get();
		}

		TType* operator->()
		{
			return &(get());
		}

		const TType* operator->() const
		{
			return &(get());
		}

		void release() const
		{
			std::scoped_lock lock(_mutex);
			_destroyDataLocked();
		}

		void configure(Generator p_generator, Destructor p_destructor = nullptr)
		{
			std::scoped_lock lock(_mutex);
			_destroyDataLocked();
			_generator = std::move(p_generator);
			_destructor = std::move(p_destructor);
		}

		void set(const TType& p_value)
		{
			std::scoped_lock lock(_mutex);
			_destroyDataLocked();
			_data = p_value;
		}

		void set(TType&& p_value)
		{
			std::scoped_lock lock(_mutex);
			_destroyDataLocked();
			_data = std::move(p_value);
		}

		std::optional<TType> take()
		{
			std::scoped_lock lock(_mutex);

			if (_data.has_value() == false)
			{
				return std::nullopt;
			}

			std::optional<TType> result(std::move(*_data));
			_data.reset();
			return result;
		}

		bool isCached() const
		{
			std::scoped_lock lock(_mutex);
			return _data.has_value();
		}

		TType& refresh()
		{
			std::scoped_lock lock(_mutex);
			_destroyDataLocked();
			_generateDataLocked();
			return _data.value();
		}

		const TType& refresh() const
		{
			std::scoped_lock lock(_mutex);
			_destroyDataLocked();
			_generateDataLocked();
			return _data.value();
		}

		bool hasGenerator() const
		{
			std::scoped_lock lock(_mutex);
			return (_generator != nullptr);
		}
	};
}