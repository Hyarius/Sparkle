#include <cstddef>
#include <type_traits>

#include <gtest/gtest.h>

#include "type/spk_padding.hpp"

namespace
{
	struct LayoutWithPadding
	{
		float first = 0.0f;
		SPK_PADDING(3);
		float second = 0.0f;
		SPK_PADDING(1);
		float third = 0.0f;
	};

	struct LayoutWithRepeatedPaddingOnSameLine
	{
		float first = 0.0f;
		SPK_PADDING(1); SPK_PADDING(2);
		float second = 0.0f;
	};
}

TEST(Padding, type_ShouldMatchRequestedFloatStorage)
{
	static_assert(std::is_standard_layout_v<spk::Padding<3>>);
	static_assert(std::is_trivially_copyable_v<spk::Padding<3>>);
	static_assert(sizeof(spk::Padding<3>) == sizeof(float) * 3);
	static_assert(alignof(spk::Padding<3>) == alignof(float));

	const spk::Padding<3> padding;

	EXPECT_EQ(padding.values[0], 0.0f);
	EXPECT_EQ(padding.values[1], 0.0f);
	EXPECT_EQ(padding.values[2], 0.0f);
}

TEST(Padding, macro_ShouldPreserveStructLayout)
{
	static_assert(std::is_standard_layout_v<LayoutWithPadding>);
	static_assert(sizeof(LayoutWithPadding) == sizeof(float) * 7);
	static_assert(offsetof(LayoutWithPadding, first) == 0);
	static_assert(offsetof(LayoutWithPadding, second) == sizeof(float) * 4);
	static_assert(offsetof(LayoutWithPadding, third) == sizeof(float) * 6);

	EXPECT_EQ(sizeof(LayoutWithPadding), sizeof(float) * 7);
}

TEST(Padding, macro_ShouldSupportRepeatedUseOnSameLine)
{
	static_assert(std::is_standard_layout_v<LayoutWithRepeatedPaddingOnSameLine>);
	static_assert(sizeof(LayoutWithRepeatedPaddingOnSameLine) == sizeof(float) * 5);
	static_assert(offsetof(LayoutWithRepeatedPaddingOnSameLine, second) == sizeof(float) * 4);

	EXPECT_EQ(sizeof(LayoutWithRepeatedPaddingOnSameLine), sizeof(float) * 5);
}
