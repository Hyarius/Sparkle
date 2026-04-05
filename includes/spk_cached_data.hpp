#pragma once

#include <concepts>
#include <functional>
#include <optional>
#include <stdexcept>
#include <type_traits>
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

		void _destroyData() const
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

		void _generateData() const
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
			_generateData();
			return _data.value();
		}

		const TType& get() const
		{
			_generateData();
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
			_destroyData();
		}

		void configure(Generator p_generator, Destructor p_destructor = nullptr)
		{
			_destroyData();
			_generator = std::move(p_generator);
			_destructor = std::move(p_destructor);
		}

		void set(const TType& p_value)
		{
			_destroyData();
			_data = p_value;
		}

		void set(TType&& p_value)
		{
			_destroyData();
			_data = std::move(p_value);
		}

		std::optional<TType> take()
		{
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
			return _data.has_value();
		}

		TType& refresh()
		{
			_destroyData();
			_generateData();
			return _data.value();
		}

		const TType& refresh() const
		{
			_destroyData();
			_generateData();
			return _data.value();
		}

		bool hasGenerator() const
		{
			return (_generator != nullptr);
		}
	};
}