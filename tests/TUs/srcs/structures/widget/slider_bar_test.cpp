#include <gtest/gtest.h>

#include <stdexcept>

#include "structures/widget/spk_slider_bar.hpp"
#include "structures/widget/spk_widget_visual_test_helpers.hpp"
#include "structures/application/module/spk_mouse_module.hpp"
#include "structures/system/device/window/window_test_utils.hpp"

TEST(SliderBarTest, DefaultStateIsHorizontalWithNullRatio)
{
	spk::SliderBar slider("Slider");

	EXPECT_EQ(slider.orientation(), spk::Orientation::Horizontal);
	EXPECT_FLOAT_EQ(slider.ratio(), 0.0f);
	EXPECT_FLOAT_EQ(slider.scale(), 0.1f);
	EXPECT_FLOAT_EQ(slider.minValue(), 0.0f);
	EXPECT_FLOAT_EQ(slider.maxValue(), 100.0f);
	EXPECT_FALSE(slider.isDragged());
	EXPECT_TRUE(slider.isActivated());
}

TEST(SliderBarTest, GeometryPositionsBodyAccordingToRatio)
{
	spk::SliderBar slider("Slider");
	slider.setGeometry(spk::Rect2D(0, 0, 100, 20));

	EXPECT_EQ(slider.body().geometry(), spk::Rect2D(0, 0, 20, 20));

	slider.setRatio(1.0f);

	EXPECT_EQ(slider.body().geometry(), spk::Rect2D(80, 0, 20, 20));
}

TEST(SliderBarTest, SetRatioClampsAndTriggersContract)
{
	spk::SliderBar slider("Slider");

	float lastRatio = -1.0f;
	auto contract = slider.subscribeToEdition([&lastRatio](float p_ratio)
	{
		lastRatio = p_ratio;
	});

	slider.setRatio(2.0f);

	EXPECT_FLOAT_EQ(slider.ratio(), 1.0f);
	EXPECT_FLOAT_EQ(lastRatio, 1.0f);
}

TEST(SliderBarTest, SetValueConvertsToRatio)
{
	spk::SliderBar slider("Slider");
	slider.setRange(0.0f, 200.0f);

	slider.setValue(50.0f);

	EXPECT_FLOAT_EQ(slider.ratio(), 0.25f);
	EXPECT_FLOAT_EQ(slider.value(), 50.0f);
}

TEST(SliderBarTest, SetScaleRejectsOutOfRangeValues)
{
	spk::SliderBar slider("Slider");

	EXPECT_THROW(slider.setScale(0.0f), std::invalid_argument);
	EXPECT_THROW(slider.setScale(1.5f), std::invalid_argument);
}

TEST(SliderBarTest, SetRangeRejectsInvertedBounds)
{
	spk::SliderBar slider("Slider");

	EXPECT_THROW(slider.setRange(10.0f, 0.0f), std::invalid_argument);
}

TEST(SliderBarTest, DraggingBodyUpdatesRatio)
{
	spk::SliderBar slider("Slider");
	slider.setGeometry(spk::Rect2D(0, 0, 100, 20));

	float lastRatio = -1.0f;
	auto contract = slider.subscribeToEdition([&lastRatio](float p_ratio)
	{
		lastRatio = p_ratio;
	});

	spk::MouseModule mouseModule;
	mouseModule.bind(&slider);
	mouseModule.pushEvent(spk::MouseEventRecord(spk::makeEventRecord(spk::MouseMovedRecord{.position = {5, 10}})));
	mouseModule.pushEvent(spk::MouseEventRecord(spk::makeEventRecord(spk::MouseButtonPressedRecord{.button = spk::Mouse::Left})));
	mouseModule.processEvents();

	EXPECT_TRUE(slider.isDragged());

	mouseModule.pushEvent(spk::MouseEventRecord(spk::makeEventRecord(spk::MouseMovedRecord{.position = {50, 10}})));
	mouseModule.pushEvent(spk::MouseEventRecord(spk::makeEventRecord(spk::MouseButtonReleasedRecord{.button = spk::Mouse::Left})));
	mouseModule.processEvents();

	EXPECT_FALSE(slider.isDragged());
	EXPECT_FLOAT_EQ(slider.ratio(), 45.0f / 80.0f);
	EXPECT_FLOAT_EQ(lastRatio, 45.0f / 80.0f);
}

TEST(SliderBarTest, PressOnTrackJumpsToPositionAndStartsDrag)
{
	spk::SliderBar slider("Slider");
	slider.setGeometry(spk::Rect2D(0, 0, 100, 20));

	spk::MouseModule mouseModule;
	mouseModule.bind(&slider);
	mouseModule.pushEvent(spk::MouseEventRecord(spk::makeEventRecord(spk::MouseMovedRecord{.position = {80, 10}})));
	mouseModule.pushEvent(spk::MouseEventRecord(spk::makeEventRecord(spk::MouseButtonPressedRecord{.button = spk::Mouse::Left})));
	mouseModule.processEvents();

	EXPECT_TRUE(slider.isDragged());
	EXPECT_NEAR(slider.ratio(), (80.0f - 10.0f) / 80.0f, 0.001f);

	mouseModule.pushEvent(spk::MouseEventRecord(spk::makeEventRecord(spk::MouseButtonReleasedRecord{.button = spk::Mouse::Left})));
	mouseModule.processEvents();

	EXPECT_FALSE(slider.isDragged());
}

TEST(SliderBarTest, PressOutsideWidgetDoesNotStartDrag)
{
	spk::SliderBar slider("Slider");
	slider.setGeometry(spk::Rect2D(0, 0, 100, 20));

	spk::MouseModule mouseModule;
	mouseModule.bind(&slider);
	mouseModule.pushEvent(spk::MouseEventRecord(spk::makeEventRecord(spk::MouseMovedRecord{.position = {200, 200}})));
	mouseModule.pushEvent(spk::MouseEventRecord(spk::makeEventRecord(spk::MouseButtonPressedRecord{.button = spk::Mouse::Left})));
	mouseModule.processEvents();

	EXPECT_FALSE(slider.isDragged());
}

TEST(SliderBarTest, VerticalOrientationMovesBodyAlongHeight)
{
	spk::SliderBar slider("Slider", spk::Orientation::Vertical);
	slider.setGeometry(spk::Rect2D(0, 0, 20, 100));
	slider.setRatio(1.0f);

	EXPECT_EQ(slider.orientation(), spk::Orientation::Vertical);
	EXPECT_EQ(slider.body().geometry(), spk::Rect2D(0, 80, 20, 20));
}

TEST(SliderBarVisualTest, RendersHorizontalAtZero)
{
	const spk::Rect2D captureRect(0, 0, 240, 32);

	spk::SliderBar slider("Slider", spk::Orientation::Horizontal);
	slider.setRatio(0.0f);

	const sparkle_test::ImageComparisonResult result =
		spk::test::compareSnapshot(slider, "SliderBarVisual", "horizontal_zero", captureRect);

	EXPECT_TRUE(result.matches);
}

TEST(SliderBarVisualTest, RendersHorizontalAtMiddle)
{
	const spk::Rect2D captureRect(0, 0, 240, 32);

	spk::SliderBar slider("Slider", spk::Orientation::Horizontal);
	slider.setRatio(0.5f);

	const sparkle_test::ImageComparisonResult result =
		spk::test::compareSnapshot(slider, "SliderBarVisual", "horizontal_middle", captureRect);

	EXPECT_TRUE(result.matches);
}

TEST(SliderBarVisualTest, RendersHorizontalAtFull)
{
	const spk::Rect2D captureRect(0, 0, 240, 32);

	spk::SliderBar slider("Slider", spk::Orientation::Horizontal);
	slider.setRatio(1.0f);

	const sparkle_test::ImageComparisonResult result =
		spk::test::compareSnapshot(slider, "SliderBarVisual", "horizontal_full", captureRect);

	EXPECT_TRUE(result.matches);
}

TEST(SliderBarVisualTest, RendersVerticalAtMiddle)
{
	const spk::Rect2D captureRect(0, 0, 32, 240);

	spk::SliderBar slider("Slider", spk::Orientation::Vertical);
	slider.setRatio(0.5f);

	const sparkle_test::ImageComparisonResult result =
		spk::test::compareSnapshot(slider, "SliderBarVisual", "vertical_middle", captureRect);

	EXPECT_TRUE(result.matches);
}

TEST(SliderBarVisualTest, RendersLargeScaleBody)
{
	const spk::Rect2D captureRect(0, 0, 240, 32);

	spk::SliderBar slider("Slider", spk::Orientation::Horizontal);
	slider.setScale(0.5f);
	slider.setRatio(0.5f);

	const sparkle_test::ImageComparisonResult result =
		spk::test::compareSnapshot(slider, "SliderBarVisual", "large_scale_middle", captureRect);

	EXPECT_TRUE(result.matches);
}
