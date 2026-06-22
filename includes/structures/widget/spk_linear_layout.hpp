#pragma once

#include <algorithm>
#include <limits>
#include <vector>

#include "structures/widget/spk_layout.hpp"
#include "type/spk_orientation.hpp"

namespace spk
{
	template <spk::Orientation TOrientation>
	class LinearLayout : public spk::Layout
	{
	private:
		static constexpr bool _horizontalMode = (TOrientation == spk::Orientation::Horizontal);

		template <typename TVector>
		static int _primary(const TVector &p_vector)
		{
			if constexpr (_horizontalMode)
			{
				return static_cast<int>(p_vector.x);
			}
			else
			{
				return static_cast<int>(p_vector.y);
			}
		}

		template <typename TVector>
		static int _secondary(const TVector &p_vector)
		{
			if constexpr (_horizontalMode)
			{
				return static_cast<int>(p_vector.y);
			}
			else
			{
				return static_cast<int>(p_vector.x);
			}
		}

		struct Item
		{
			Element *elt = nullptr;
			int size = 0;
			int minP = 0;
			int maxP = std::numeric_limits<int>::max();
			int minS = 0;
			int maxS = std::numeric_limits<int>::max();
			bool extend = false;
		};

		static bool _isValidElement(const Element *p_element)
		{
			return p_element != nullptr && (p_element->widget() != nullptr || p_element->layout() != nullptr);
		}

		static int _saturateToInt(const std::uint64_t p_value)
		{
			constexpr int maxInt = std::numeric_limits<int>::max();

			if (p_value > static_cast<std::uint64_t>(maxInt))
			{
				return maxInt;
			}

			return static_cast<int>(p_value);
		}

		static int _saturatedPrimary(const spk::Vector2UInt &p_vector)
		{
			if constexpr (_horizontalMode)
			{
				return _saturateToInt(p_vector.x);
			}
			else
			{
				return _saturateToInt(p_vector.y);
			}
		}

		static int _saturatedSecondary(const spk::Vector2UInt &p_vector)
		{
			if constexpr (_horizontalMode)
			{
				return _saturateToInt(p_vector.y);
			}
			else
			{
				return _saturateToInt(p_vector.x);
			}
		}

		static void _applyExtendPolicy(Item &p_item, const int p_requestedSize, const bool p_shouldExtend)
		{
			p_item.extend = p_shouldExtend;
			p_item.size = p_shouldExtend ? p_requestedSize : p_item.minP;
		}

		static void _applySizePolicy(Item &p_item, const SizePolicy p_sizePolicy, const int p_requestedSize)
		{
			switch (p_sizePolicy)
			{
			case SizePolicy::Fixed:
				p_item.size = p_requestedSize;
				break;

			case SizePolicy::Minimum:
				p_item.size = p_item.minP;
				break;

			case SizePolicy::Maximum:
				p_item.size = p_item.maxP;
				break;

			case SizePolicy::Extend:
				_applyExtendPolicy(p_item, p_requestedSize, true);
				break;

			case SizePolicy::HorizontalExtend:
				_applyExtendPolicy(p_item, p_requestedSize, _horizontalMode);
				break;

			case SizePolicy::VerticalExtend:
				_applyExtendPolicy(p_item, p_requestedSize, !_horizontalMode);
				break;
			}
		}

		static Item _makeItem(Element *p_element)
		{
			const spk::Vector2UInt requestedSize = p_element->size();
			const spk::Vector2UInt minimalSize = p_element->minimalSize();
			const spk::Vector2UInt maximalSize = p_element->maximalSize();

			Item result;
			result.elt = p_element;

			result.minP = _saturatedPrimary(minimalSize);
			result.maxP = std::max(result.minP, _saturatedPrimary(maximalSize));

			result.minS = _saturatedSecondary(minimalSize);
			result.maxS = std::max(result.minS, _saturatedSecondary(maximalSize));

			_applySizePolicy(result, p_element->sizePolicy(), _saturatedPrimary(requestedSize));

			result.size = std::clamp(result.size, result.minP, result.maxP);

			return result;
		}

		std::vector<Item> _collectItems() const
		{
			std::vector<Item> result;
			result.reserve(_elements.size());

			for (const auto &elementPointer : _elements)
			{
				Element *element = elementPointer.get();

				if (_isValidElement(element) == false)
				{
					continue;
				}

				result.push_back(_makeItem(element));
			}

			return result;
		}

		static int _sumBase(const std::vector<Item> &p_items)
		{
			int sum = 0;
			for (const auto &it : p_items)
			{
				sum += it.size;
			}
			return sum;
		}

		static void _distributeExtra(std::vector<Item> &p_items, int p_extra)
		{
			if (p_extra <= 0 || p_items.empty() == true)
			{
				return;
			}

			size_t extendCount = 0;
			for (auto &it : p_items)
			{
				if (it.extend == true)
				{
					++extendCount;
				}
			}

			if (extendCount == 0)
			{
				for (auto &it : p_items)
				{
					it.extend = true;
				}
				extendCount = p_items.size();
			}

			const int share = p_extra / static_cast<int>(extendCount);
			int remain = p_extra % static_cast<int>(extendCount);

			for (auto &it : p_items)
			{
				if (it.extend == false)
				{
					continue;
				}

				int add = share + (remain > 0 ? 1 : 0);
				if (remain > 0)
				{
					--remain;
				}

				it.size = std::clamp(it.size + add, it.minP, it.maxP);
			}
		}

		void _applyGeometry(const std::vector<Item> &p_items, const spk::Rect2D &p_geometry, int p_padding)
		{
			int cursor = _primary(p_geometry.anchor);

			for (size_t i = 0; i < p_items.size(); ++i)
			{
				const Item &it = p_items[i];

				if (i > 0)
				{
					cursor += p_padding;
				}

				const int secondarySize = std::clamp(_secondary(p_geometry.size), it.minS, it.maxS);

				spk::Rect2D cell;
				if constexpr (_horizontalMode)
				{
					cell = spk::Rect2D(
						cursor,
						p_geometry.anchor.y,
						static_cast<unsigned int>(std::max(0, it.size)),
						static_cast<unsigned int>(std::max(0, secondarySize)));
				}
				else
				{
					cell = spk::Rect2D(
						p_geometry.anchor.x,
						cursor,
						static_cast<unsigned int>(std::max(0, secondarySize)),
						static_cast<unsigned int>(std::max(0, it.size)));
				}

				it.elt->setGeometry(cell);

				cursor += it.size;
			}
		}

	private:
		[[nodiscard]] spk::Vector2UInt _computeMinimalSize() const
		{
			if (_elements.empty() == true)
			{
				return {0u, 0u};
			}

			uint32_t primarySum = 0;
			uint32_t secondaryMax = 0;
			size_t count = 0;

			for (const auto &elementPtr : _elements)
			{
				if (elementPtr == nullptr || (elementPtr->widget() == nullptr && elementPtr->layout() == nullptr))
				{
					continue;
				}

				++count;
				const spk::Vector2UInt min = elementPtr->minimalSize();

				if constexpr (_horizontalMode)
				{
					primarySum += min.x;
					secondaryMax = std::max(secondaryMax, min.y);
				}
				else
				{
					primarySum += min.y;
					secondaryMax = std::max(secondaryMax, min.x);
				}
			}

			if (count > 1)
			{
				const uint32_t pad = static_cast<uint32_t>(_primary(_elementPadding));
				primarySum += pad * static_cast<uint32_t>(count - 1);
			}

			if constexpr (_horizontalMode)
			{
				return {primarySum, secondaryMax};
			}
			else
			{
				return {secondaryMax, primarySum};
			}
		}

	public:
		LinearLayout()
		{
			sizeHint().configureMinimalGenerator([this]() {
				return _computeMinimalSize();
			});
			sizeHint().configureDesiredGenerator([this]() {
				return _computeMinimalSize();
			});
		}

		void setGeometry(const spk::Rect2D &p_geometry) override
		{
			if (_elements.empty() == true)
			{
				return;
			}

			std::vector<Item> items = _collectItems();
			if (items.empty() == true)
			{
				return;
			}

			const int pad = _primary(_elementPadding);
			const int paddingTotal = static_cast<int>(items.size() - 1) * pad;
			const int availablePrimary = _primary(p_geometry.size);
			const int baseSum = _sumBase(items);
			const int extra = std::max(0, availablePrimary - (baseSum + paddingTotal));

			_distributeExtra(items, extra);
			_applyGeometry(items, p_geometry, pad);
		}
	};

	using HorizontalLayout = LinearLayout<spk::Orientation::Horizontal>;
	using VerticalLayout = LinearLayout<spk::Orientation::Vertical>;
}
