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
			int secondarySize = 0;
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

		static spk::Vector2UInt _sizeFromPrimarySecondary(const int p_primary, const int p_secondary)
		{
			const unsigned int primary = static_cast<unsigned int>(std::max(0, p_primary));
			const unsigned int secondary = static_cast<unsigned int>(std::max(0, p_secondary));

			if constexpr (_horizontalMode)
			{
				return {primary, secondary};
			}
			else
			{
				return {secondary, primary};
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

		static Item _makeItem(Element *p_element, const int p_remainingPrimary, const int p_availableSecondary)
		{
			const spk::Vector2UInt requestedSize = p_element->size();
			const spk::Vector2UInt maximalSize = p_element->maximalSize();

			const int maxP = _saturatedPrimary(maximalSize);
			const int maxS = _saturatedSecondary(maximalSize);
			const int queryPrimary = std::clamp(p_remainingPrimary, 0, maxP);
			const int querySecondary = std::clamp(p_availableSecondary, 0, maxS);
			const spk::Vector2UInt preferredSize =
				p_element->preferredSizeFor(_sizeFromPrimarySecondary(queryPrimary, querySecondary));

			Item result;
			result.elt = p_element;

			result.minP = _saturatedPrimary(preferredSize);
			result.maxP = std::max(result.minP, maxP);

			result.minS = _saturatedSecondary(preferredSize);
			result.maxS = std::max(result.minS, maxS);

			_applySizePolicy(result, p_element->sizePolicy(), _saturatedPrimary(requestedSize));

			result.size = std::clamp(result.size, result.minP, result.maxP);
			result.secondarySize = result.minS;

			return result;
		}

		std::vector<Item> _collectItems(const spk::Vector2UInt &p_availableSize) const
		{
			std::vector<Item> result;
			result.reserve(_elements.size());

			const int paddingTotal = static_cast<int>((_elements.empty() == false ? _elements.size() - 1 : 0) * _primary(_elementPadding));
			int remainingPrimary = std::max(0, _primary(p_availableSize) - paddingTotal);
			const int availableSecondary = std::max(0, _secondary(p_availableSize));

			for (const auto &elementPointer : _elements)
			{
				Element *element = elementPointer.get();

				if (_isValidElement(element) == false)
				{
					continue;
				}

				result.push_back(_makeItem(element, remainingPrimary, availableSecondary));
				remainingPrimary = std::max(0, remainingPrimary - result.back().size);
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

		static void _refreshMinimalSecondarySizes(std::vector<Item> &p_items, const int p_availableSecondary)
		{
			for (Item &item : p_items)
			{
				const spk::Vector2UInt maximalSize = item.elt->maximalSize();
				const int maxS = _saturatedSecondary(maximalSize);
				const int querySecondary = std::clamp(p_availableSecondary, 0, maxS);
				const spk::Vector2UInt preferredSize =
					item.elt->preferredSizeFor(_sizeFromPrimarySecondary(item.size, querySecondary));

				item.minS = _saturatedSecondary(preferredSize);
				item.maxS = std::max(item.minS, maxS);
				item.secondarySize = item.minS;
			}
		}

		static void _refreshGeometrySecondarySizes(std::vector<Item> &p_items, const int p_availableSecondary)
		{
			_refreshMinimalSecondarySizes(p_items, p_availableSecondary);

			for (Item &item : p_items)
			{
				item.secondarySize = std::clamp(p_availableSecondary, item.minS, item.maxS);
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

				spk::Rect2D cell;
				if constexpr (_horizontalMode)
				{
					cell = spk::Rect2D(
						cursor,
						p_geometry.anchor.y,
						static_cast<unsigned int>(std::max(0, it.size)),
						static_cast<unsigned int>(std::max(0, it.secondarySize)));
				}
				else
				{
					cell = spk::Rect2D(
						p_geometry.anchor.x,
						cursor,
						static_cast<unsigned int>(std::max(0, it.secondarySize)),
						static_cast<unsigned int>(std::max(0, it.size)));
				}

				it.elt->setGeometry(cell);

				cursor += it.size;
			}
		}

	private:
		[[nodiscard]] spk::Vector2UInt _computePreferredSizeFor(const spk::Vector2UInt &p_availableSize) const
		{
			if (_elements.empty() == true)
			{
				return {0u, 0u};
			}

			std::vector<Item> items = _collectItems(p_availableSize);
			if (items.empty() == true)
			{
				return {0u, 0u};
			}

			uint32_t primarySum = 0;
			uint32_t secondaryMax = 0;

			_refreshMinimalSecondarySizes(items, std::max(0, _secondary(p_availableSize)));

			for (const Item &item : items)
			{
				primarySum += static_cast<uint32_t>(std::max(0, item.size));
				secondaryMax = std::max(secondaryMax, static_cast<uint32_t>(std::max(0, item.secondarySize)));
			}

			if (items.size() > 1)
			{
				const uint32_t pad = static_cast<uint32_t>(_primary(_elementPadding));
				primarySum += pad * static_cast<uint32_t>(items.size() - 1);
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

		[[nodiscard]] spk::Vector2UInt _computeMinimalSize() const
		{
			const spk::Vector2UInt unbounded = std::numeric_limits<spk::Vector2UInt>::max();
			return {
				_computePreferredSizeFor({0u, unbounded.y}).x,
				_computePreferredSizeFor(unbounded).y};
		}

	public:
		LinearLayout()
		{
			configureMinimalSizeGenerator([this]() {
				return _computeMinimalSize();
			});
			configureFixedSizeGenerator([this]() {
				return _computeMinimalSize();
			});
		}

		[[nodiscard]] spk::Vector2UInt preferredSizeFor(const spk::Vector2UInt &p_availableSize) const override
		{
			return _computePreferredSizeFor(p_availableSize);
		}

		void setGeometry(const spk::Rect2D &p_geometry) override
		{
			if (_elements.empty() == true)
			{
				return;
			}

			std::vector<Item> items = _collectItems(p_geometry.size);
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
			_refreshGeometrySecondarySizes(items, std::max(0, _secondary(p_geometry.size)));
			_applyGeometry(items, p_geometry, pad);
		}
	};

	using HorizontalLayout = LinearLayout<spk::Orientation::Horizontal>;
	using VerticalLayout = LinearLayout<spk::Orientation::Vertical>;
}
