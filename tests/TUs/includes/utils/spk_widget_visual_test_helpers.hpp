#pragma once

#include <string>

#include "image_comparison_test_utils.hpp"
#include "math/spk_rect_2d.hpp"
#include "test_resource_path_utils.hpp"
#include "widget/spk_widget.hpp"

namespace spk::test
{
	[[nodiscard]] sparkle_test::ImageComparisonResult compareSnapshot(
		spk::Widget& p_widget,
		const std::string& p_widgetName,
		const std::string& p_variant,
		const spk::Rect2D& p_captureRect);
}
