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
		class SizeHint
		{
		public:
			using Generator = spk::CachedData<spk::Vector2UInt>::Generator;

		private:
			mutable spk::CachedData<spk::Vector2UInt> _minimal{[]() { return spk::Vector2UInt(0, 0); }};
			mutable spk::CachedData<spk::Vector2UInt> _desired{[]() { return spk::Vector2UInt(0, 0); }};
			mutable spk::CachedData<spk::Vector2UInt> _maximal{[]() { return std::numeric_limits<spk::Vector2UInt>::max(); }};

		public:
			SizeHint() = default;
			SizeHint(Generator p_minimalGenerator, Generator p_desiredGenerator, Generator p_maximalGenerator);

			void configure(Generator p_minimalGenerator, Generator p_desiredGenerator, Generator p_maximalGenerator);
			void configureMinimalGenerator(Generator p_generator);
			void configureDesiredGenerator(Generator p_generator);
			void configureMaximalGenerator(Generator p_generator);

			void release() const;
			void releaseMinimal() const;
			void releaseDesired() const;
			void releaseMaximal() const;

			void setMinimal(const spk::Vector2UInt& p_minimalValue);
			void setDesired(const spk::Vector2UInt& p_desiredValue);
			void setMaximal(const spk::Vector2UInt& p_maximalValue);

			[[nodiscard]] const spk::Vector2UInt& minimal() const;
			[[nodiscard]] const spk::Vector2UInt& desired() const;
			[[nodiscard]] const spk::Vector2UInt& maximal() const;
		};

	private:
		mutable SizeHint _sizeHint;

	public:
		ResizableElement() = default;
		virtual ~ResizableElement() = default;

		virtual void setGeometry(const spk::Rect2D& p_geometry) = 0;

		[[nodiscard]] SizeHint& sizeHint();
		[[nodiscard]] const SizeHint& sizeHint() const;
	};
}
