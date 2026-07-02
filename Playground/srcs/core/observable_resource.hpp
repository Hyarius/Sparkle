#pragma once

#include "structures/design_pattern/spk_contract_provider.hpp"

#include <algorithm>
#include <cstddef>
#include <functional>
#include <stdexcept>
#include <utility>
#include <vector>

namespace pg
{
	namespace detail
	{
		// ContractProvider stores decayed trigger arguments for deferred delivery. A
		// pointer payload keeps the CRTP type complete-safe; subscribe() adapts it back
		// to the public const-reference callback specified by the Playground API.
		template <typename TValue, typename TDerived>
		class BasicObservableResource : private spk::ContractProvider<const TDerived *>
		{
		private:
			using Provider = spk::ContractProvider<const TDerived *>;
			using Provider::trigger;

			TValue _current{};
			TValue _max{};

			void _assign(TValue p_current, TValue p_max, bool p_force)
			{
				const bool changed = _current != p_current || _max != p_max;
				if (!changed && !p_force)
				{
					return;
				}

				_current = p_current;
				_max = p_max;
				trigger(&static_cast<const TDerived &>(*this));
			}

		protected:
			BasicObservableResource(TValue p_current, TValue p_max)
			{
				_max = std::max(TValue{}, p_max);
				_current = std::clamp(p_current, TValue{}, _max);
			}

		public:
			using Contract = typename Provider::Contract;
			using Callback = std::function<void(const TDerived &)>;

			[[nodiscard]] Contract subscribe(Callback p_callback)
			{
				return Provider::subscribe([callback = std::move(p_callback)](const TDerived *p_resource) {
					callback(*p_resource);
				});
			}

			[[nodiscard]] TValue current() const noexcept
			{
				return _current;
			}

			[[nodiscard]] TValue max() const noexcept
			{
				return _max;
			}

			void set(TValue p_current, TValue p_max, bool p_force = false)
			{
				const TValue clampedMax = std::max(TValue{}, p_max);
				_assign(std::clamp(p_current, TValue{}, clampedMax), clampedMax, p_force);
			}

			void setCurrent(TValue p_current, bool p_force = false)
			{
				set(p_current, _max, p_force);
			}

			void setCurrentAllowOverflow(TValue p_current, bool p_force = false)
			{
				_assign(std::max(TValue{}, p_current), _max, p_force);
			}

			void setMax(TValue p_max, bool p_force = false)
			{
				set(_current, p_max, p_force);
			}

			void change(TValue p_amount, bool p_force = false)
			{
				setCurrent(_current + p_amount, p_force);
			}

			void increase(TValue p_amount, bool p_force = false)
			{
				change(p_amount, p_force);
			}

			void decrease(TValue p_amount, bool p_force = false)
			{
				change(-p_amount, p_force);
			}

			void reset(bool p_force = false)
			{
				setCurrent(_max, p_force);
			}

			[[nodiscard]] float ratio() const noexcept
			{
				if (_max <= TValue{})
				{
					return 0.0f;
				}
				return static_cast<float>(_current) / static_cast<float>(_max);
			}
		};
	}

	class ObservableResource : public detail::BasicObservableResource<int, ObservableResource>
	{
	public:
		ObservableResource() :
			detail::BasicObservableResource<int, ObservableResource>(0, 0)
		{
		}

		ObservableResource(int p_current, int p_max) :
			detail::BasicObservableResource<int, ObservableResource>(p_current, p_max)
		{
		}
	};

	class ObservableFloatResource : public detail::BasicObservableResource<float, ObservableFloatResource>
	{
	public:
		ObservableFloatResource() :
			detail::BasicObservableResource<float, ObservableFloatResource>(0.0f, 0.0f)
		{
		}

		ObservableFloatResource(float p_current, float p_max) :
			detail::BasicObservableResource<float, ObservableFloatResource>(p_current, p_max)
		{
		}
	};

	template <typename TItem>
	class ObservableList : private spk::ContractProvider<const ObservableList<TItem> *>
	{
	private:
		using Provider = spk::ContractProvider<const ObservableList<TItem> *>;
		using Provider::trigger;

		std::vector<TItem> _items;

		void _checkIndex(std::size_t p_index) const
		{
			if (p_index >= _items.size())
			{
				throw std::out_of_range("ObservableList index out of range");
			}
		}

	public:
		using Contract = typename Provider::Contract;
		using Callback = std::function<void(const ObservableList<TItem> &)>;

		ObservableList() = default;

		explicit ObservableList(std::vector<TItem> p_items) :
			_items(std::move(p_items))
		{
		}

		[[nodiscard]] Contract subscribe(Callback p_callback)
		{
			return Provider::subscribe([callback = std::move(p_callback)](const ObservableList<TItem> *p_list) {
				callback(*p_list);
			});
		}

		[[nodiscard]] std::size_t size() const noexcept
		{
			return _items.size();
		}

		[[nodiscard]] bool empty() const noexcept
		{
			return _items.empty();
		}

		[[nodiscard]] TItem &at(std::size_t p_index)
		{
			return _items.at(p_index);
		}

		[[nodiscard]] const TItem &at(std::size_t p_index) const
		{
			return _items.at(p_index);
		}

		[[nodiscard]] TItem &operator[](std::size_t p_index)
		{
			return _items[p_index];
		}

		[[nodiscard]] const TItem &operator[](std::size_t p_index) const
		{
			return _items[p_index];
		}

		[[nodiscard]] const std::vector<TItem> &items() const noexcept
		{
			return _items;
		}

		[[nodiscard]] auto begin() noexcept
		{
			return _items.begin();
		}

		[[nodiscard]] auto end() noexcept
		{
			return _items.end();
		}

		[[nodiscard]] auto begin() const noexcept
		{
			return _items.begin();
		}

		[[nodiscard]] auto end() const noexcept
		{
			return _items.end();
		}

		void add(const TItem &p_item)
		{
			_items.push_back(p_item);
			trigger(this);
		}

		void add(TItem &&p_item)
		{
			_items.push_back(std::move(p_item));
			trigger(this);
		}

		void removeAt(std::size_t p_index)
		{
			_checkIndex(p_index);
			_items.erase(_items.begin() + static_cast<std::ptrdiff_t>(p_index));
			trigger(this);
		}

		void clear()
		{
			if (_items.empty())
			{
				return;
			}
			_items.clear();
			trigger(this);
		}

		void notifyItemChangedAt(std::size_t p_index)
		{
			_checkIndex(p_index);
			trigger(this);
		}
	};
}
