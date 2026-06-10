#include <gtest/gtest.h>

#include "structures/widget/spk_animation_label.hpp"
#include "structures/widget/spk_image_label.hpp"
#include "structures/widget/spk_widget_style.hpp"
#include "structures/widget/spk_widget_visual_test_helpers.hpp"

namespace
{
	[[nodiscard]] std::shared_ptr<spk::SpriteSheet> makeTestSpriteSheet()
	{
		return spk::WidgetStyle::makeDefault().nineSliceSpriteSheet();
	}
}

TEST(ImageLabelVisualTest, RendersEmptyWithNoTexture)
{
	const spk::Rect2D captureRect(0, 0, 96, 96);

	spk::ImageLabel label("Image");

	const sparkle_test::ImageComparisonResult result =
		spk::test::compareSnapshot(label, "ImageLabelVisual", "no_texture", captureRect);

	EXPECT_TRUE(result.matches);
}

TEST(ImageLabelVisualTest, RendersFullSpriteSheet)
{
	const spk::Rect2D captureRect(0, 0, 96, 96);

	auto sheet = makeTestSpriteSheet();
	spk::ImageLabel label("Image", sheet);

	const sparkle_test::ImageComparisonResult result =
		spk::test::compareSnapshot(label, "ImageLabelVisual", "full_sheet", captureRect);

	EXPECT_TRUE(result.matches);
}

TEST(ImageLabelVisualTest, RendersSingleSprite)
{
	const spk::Rect2D captureRect(0, 0, 96, 96);

	auto sheet = makeTestSpriteSheet();
	spk::ImageLabel label("Image", sheet);
	label.setSection(sheet->sprite(spk::Vector2UInt{1, 1}));

	const sparkle_test::ImageComparisonResult result =
		spk::test::compareSnapshot(label, "ImageLabelVisual", "single_sprite_center", captureRect);

	EXPECT_TRUE(result.matches);
}

TEST(AnimationLabelVisualTest, RendersFirstFrame)
{
	const spk::Rect2D captureRect(0, 0, 96, 96);

	spk::AnimationLabel label("Anim", makeTestSpriteSheet());

	const sparkle_test::ImageComparisonResult result =
		spk::test::compareSnapshot(label, "AnimationLabelVisual", "frame_0", captureRect);

	EXPECT_TRUE(result.matches);
}

TEST(AnimationLabelVisualTest, RendersSecondFrame)
{
	const spk::Rect2D captureRect(0, 0, 96, 96);

	spk::AnimationLabel label("Anim", makeTestSpriteSheet());
	label.setLoopSpeed(spk::Duration(0.0L, spk::TimeUnit::Millisecond));
	spk::UpdateTick tick{};
	label.update(tick);

	const sparkle_test::ImageComparisonResult result =
		spk::test::compareSnapshot(label, "AnimationLabelVisual", "frame_1", captureRect);

	EXPECT_TRUE(result.matches);
}
