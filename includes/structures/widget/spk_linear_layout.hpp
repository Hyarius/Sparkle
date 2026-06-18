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
		static int _primary(const TVector& p_vector)
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
		static int _secondary(const TVector& p_vector)
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
			Element* elt = nullptr;
			int size = 0;
			int minP = 0;
			int maxP = std::numeric_limits<int>::max();
			int minS = 0;
			int maxS = std::numeric_limits<int>::max();
			bool extend = false;
		};

		std::vector<Item> _collectItems() const
		{
			std::vector<Item> out;
			out.reserve(_elements.size());

			for (const auto& up : _elements)
			{
				Element* e = up.get();

				if (e == nullptr || (e->widget() == nullptr && e->layout() == nullptr))
				{
					continue;
				}

				const spk::Vector2UInt requestedSize = e->size();
				const spk::Vector2UInt minSize = e->minimalSize();
				const spk::Vector2UInt maxSize = e->maximalSize();

				Item it;
				it.elt = e;

				it.minP = std::max(0, _primary(minSize));
				const long long rawMaxP = static_cast<long long>(_horizontalMode ? maxSize.x : maxSize.y);
				it.maxP =
					(rawMaxP > static_cast<long long>(std::numeric_limits<int>::max())) ? std::numeric_limits<int>::max() : static_cast<int>(rawMaxP);
				if (it.maxP < it.minP)
				{
					it.maxP = it.minP;
				}

				it.minS = std::max(0, _secondary(minSize));
				const long long rawMaxS = static_cast<long long>(_horizontalMode ? maxSize.y : maxSize.x);
				it.maxS =
					(rawMaxS > static_cast<long long>(std::numeric_limits<int>::max())) ? std::numeric_limits<int>::max() : static_cast<int>(rawMaxS);
				if (it.maxS < it.minS)
				{
					it.maxS = it.minS;
				}

				const int requested = _primary(requestedSize);

				switch (e->sizePolicy())
				{
				case SizePolicy::Fixed:
					it.size = requested;
					break;

				case SizePolicy::Minimum:
					it.size = it.minP;
					break;

				case SizePolicy::Maximum:
					it.size = it.maxP;
					break;

				case SizePolicy::Extend:
					it.extend = true;
					it.size = requested;
					break;

				case SizePolicy::HorizontalExtend:
					if constexpr (_horizontalMode)
					{
						it.extend = true;
						it.size = requested;
					}
					else
					{
						it.size = it.minP;
					}
					break;

				case SizePolicy::VerticalExtend:
					if constexpr (!_horizontalMode)
					{
						it.extend = true;
						it.size = requested;
					}
					else
					{
						it.size = it.minP;
					}
					break;
				}

				it.size = std::clamp(it.size, it.minP, it.maxP);

				out.push_back(it);
			}

			return out;
		}

		static int _sumBase(const std::vector<Item>& p_items)
		{
			int sum = 0;
			for (const auto& it : p_items)
			{
				sum += it.size;
			}
			return sum;
		}

		static void _distributeExtra(std::vector<Item>& p_items, int p_extra)
		{
			if (p_extra <= 0 || p_items.empty() == true)
			{
				return;
			}

			size_t extendCount = 0;
			for (auto& it : p_items)
			{
				if (it.extend == true)
				{
					++extendCount;
				}
			}

			if (extendCount == 0)
			{
				for (auto& it : p_items)
				{
					it.extend = true;
				}
				extendCount = p_items.size();
			}

			const int share = p_extra / static_cast<int>(extendCount);
			int remain = p_extra % static_cast<int>(extendCount);

			for (auto& it : p_items)
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

		void _applyGeometry(const std::vector<Item>& p_items, const spk::Rect2D& p_geometry, int p_padding)
		{
			int cursor = _primary(p_geometry.anchor);

			for (size_t i = 0; i < p_items.size(); ++i)
			{
				const Item& it = p_items[i];

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

			for (const auto& elementPtr : _elements)
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
			sizeHint().configureMinimalGenerator([this]() { return _computeMinimalSize(); });
			sizeHint().configureDesiredGenerator([this]() { return _computeMinimalSize(); });
		}

		void setGeometry(const spk::Rect2D& p_geometry) override
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
