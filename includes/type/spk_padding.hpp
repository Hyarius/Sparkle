#pragma once

#include <cstddef>

namespace spk
{
	template <std::size_t Size>
	struct Padding
	{
		static_assert(Size > 0, "spk::Padding must contain at least one float");

		float values[Size] = {};
	};
}

#define SPK_DETAIL_CONCAT_IMPL(p_left, p_right) p_left##p_right
#define SPK_DETAIL_CONCAT(p_left, p_right) SPK_DETAIL_CONCAT_IMPL(p_left, p_right)
#define SPK_DETAIL_PADDING_NAME_IMPL(p_line, p_counter) SPK_DETAIL_CONCAT(SPK_DETAIL_CONCAT(SPK_DETAIL_CONCAT(_spk_padding_, p_line), _), p_counter)
#define SPK_DETAIL_PADDING_NAME(p_line, p_counter) SPK_DETAIL_PADDING_NAME_IMPL(p_line, p_counter)

#define SPK_PADDING(p_size) ::spk::Padding<p_size> SPK_DETAIL_PADDING_NAME(__LINE__, __COUNTER__)
