#pragma once

#include <limits>

#include "structures/container/spk_cached_data.hpp"
#include "structures/math/spk_rect_2d.hpp"
#include "structures/math/spk_vector2.hpp"

namespace spk
{
	class ResizableElement
	{
	public:
		using SizeGenerator = spk::CachedData<spk::Vector2UInt>::Generator;

	private:
		mutable spk::CachedData<spk::Vector2UInt> _minimalSize{[]() -> spk::Vector2UInt {
			return spk::Vector2UInt(0, 0);
		}};
		mutable spk::CachedData<spk::Vector2UInt> _fixedSize{[]() -> spk::Vector2UInt {
			return spk::Vector2UInt(0, 0);
		}};
		mutable spk::CachedData<spk::Vector2UInt> _maximalSize{[]() -> spk::Vector2UInt {
			constexpr auto maximumValue = (std::numeric_limits<spk::Vector2UInt::value_type>::max)();
			return spk::Vector2UInt(maximumValue, maximumValue);
		}};

	public:
		ResizableElement() = default;
		virtual ~ResizableElement() = default;

		virtual void setGeometry(const spk::Rect2D &p_geometry) = 0;

		void configureSizeGenerators(
			SizeGenerator p_minimalGenerator,
			SizeGenerator p_fixedGenerator,
			SizeGenerator p_maximalGenerator);
		void configureMinimalSizeGenerator(SizeGenerator p_generator);
		void configureFixedSizeGenerator(SizeGenerator p_generator);
		void configureMaximalSizeGenerator(SizeGenerator p_generator);

		void releaseSizeCache() const;
		void releaseMinimalSize() const;
		void releaseFixedSize() const;
		void releaseMaximalSize() const;

		virtual void setMinimalSize(const spk::Vector2UInt &p_minimalValue);
		virtual void setFixedSize(const spk::Vector2UInt &p_fixedValue);
		virtual void setMaximalSize(const spk::Vector2UInt &p_maximalValue);

		[[nodiscard]] virtual spk::Vector2UInt minimalSize() const;
		[[nodiscard]] virtual spk::Vector2UInt minimalSizeFor(const spk::Vector2UInt &p_availableSize) const;
		[[nodiscard]] virtual spk::Vector2UInt fixedSize() const;
		[[nodiscard]] virtual spk::Vector2UInt maximalSize() const;
	};
}
