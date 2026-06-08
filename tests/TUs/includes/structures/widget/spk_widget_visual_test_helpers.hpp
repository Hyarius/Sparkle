#pragma once

#include <string>

#include "utils/image_comparison_test_utils.hpp"
#include "structures/math/spk_rect_2d.hpp"
#include "utils/test_resource_path_utils.hpp"
#include "structures/widget/spk_widget.hpp"

namespace spk::test
{
	[[nodiscard]] sparkle_test::ImageComparisonResult compareSnapshot(
		spk::Widget& p_widget,
		const std::string& p_widgetName,
		const std::string& p_variant,
		const spk::Rect2D& p_captureRect);
}
