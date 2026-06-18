#include <gtest/gtest.h>

#include <stdexcept>

#include "structures/widget/spk_animation_label.hpp"
#include "structures/widget/spk_widget_style.hpp"
#include "structures/widget/spk_widget_visual_test_helpers.hpp"

namespace
{
	[[nodiscard]] std::shared_ptr<spk::SpriteSheet> makeSpriteSheet()
	{
		return spk::WidgetStyle::makeDefault().nineSliceSpriteSheet();
	}

	[[nodiscard]] spk::UpdateTick makeTick()
	{
		return spk::UpdateTick{};
	}
}

TEST(AnimationLabelTest, NameOnlyConstructorActivatesWithoutSpriteSheet)
{
	spk::AnimationLabel label("Animation");
	label.setGeometry(spk::Rect2D(0, 0, 32, 32));

	auto renderUnit = label.renderUnit();

	ASSERT_NE(renderUnit, nullptr);
	EXPECT_EQ(renderUnit->size(), 0u);
	EXPECT_TRUE(label.isActivated());
}

TEST(AnimationLabelTest, SpriteSheetConstructorCoversFullRange)
{
	spk::AnimationLabel label("Animation", makeSpriteSheet());

	EXPECT_EQ(label.currentFrame(), 0u);
	EXPECT_EQ(label.rangeStart(), 0u);
	EXPECT_EQ(label.rangeEnd(), 8u);
}

TEST(AnimationLabelTest, BuildsSpriteRenderCommand)
{
	spk::AnimationLabel label("Animation", makeSpriteSheet());
	label.setGeometry(spk::Rect2D(0, 0, 32, 32));

	auto renderUnit = label.renderUnit();

	ASSERT_NE(renderUnit, nullptr);
	EXPECT_EQ(renderUnit->size(), 1u);
}

TEST(AnimationLabelTest, SetSpriteSheetRejectsNull)
{
	spk::AnimationLabel label("Animation");

	EXPECT_THROW(label.setSpriteSheet(nullptr), std::invalid_argument);
}

TEST(AnimationLabelTest, SetAnimationRangeRejectsInvertedRange)
{
	spk::AnimationLabel label("Animation", makeSpriteSheet());

	EXPECT_THROW(label.setAnimationRange(5, 2), std::invalid_argument);
}

TEST(AnimationLabelTest, UpdateAdvancesFrameOnceUntilTimerExpires)
{
	spk::AnimationLabel label("Animation", makeSpriteSheet());
	spk::UpdateTick tick = makeTick();

	label.update(tick);
	EXPECT_EQ(label.currentFrame(), 1u);

	label.update(tick);
	EXPECT_EQ(label.currentFrame(), 1u);
}

TEST(AnimationLabelTest, UpdateWrapsAroundAnimationRange)
{
	spk::AnimationLabel label("Animation", makeSpriteSheet());
	label.setAnimationRange(2, 3);
	label.setLoopSpeed(spk::Duration(0.0L, spk::TimeUnit::Millisecond));
	spk::UpdateTick tick = makeTick();

	EXPECT_EQ(label.currentFrame(), 2u);

	label.update(tick);
	EXPECT_EQ(label.currentFrame(), 3u);

	label.update(tick);
	EXPECT_EQ(label.currentFrame(), 2u);
}

TEST(AnimationLabelTest, UpdateWithoutSpriteSheetIsNoOp)
{
	spk::AnimationLabel label("Animation");
	spk::UpdateTick tick = makeTick();

	label.update(tick);

	EXPECT_EQ(label.currentFrame(), 0u);
}

TEST(AnimationLabelTest, SpriteSheetGetterReturnsSheet)
{
	auto sheet = makeSpriteSheet();
	spk::AnimationLabel label("Animation", sheet);

	EXPECT_EQ(label.spriteSheet(), sheet);
}

TEST(AnimationLabelTest, SetAnimationRangeUpdatesGetters)
{
	spk::AnimationLabel label("Animation", makeSpriteSheet());

	label.setAnimationRange(1, 4);

	EXPECT_EQ(label.rangeStart(), 1u);
	EXPECT_EQ(label.rangeEnd(), 4u);
	EXPECT_EQ(label.currentFrame(), 1u);
}

TEST(AnimationLabelTest, UpdateWrapsAroundFullRange)
{
	spk::AnimationLabel label("Animation", makeSpriteSheet());
	label.setLoopSpeed(spk::Duration(0.0L, spk::TimeUnit::Millisecond));
	spk::UpdateTick tick = makeTick();

	const size_t totalSprites = label.rangeEnd() + 1;
	for (size_t i = 0; i < totalSprites; ++i)
	{
		label.update(tick);
	}

	EXPECT_EQ(label.currentFrame(), label.rangeStart());
}

TEST(AnimationLabelTest, SetDepthSameValueIsNoOp)
{
	spk::AnimationLabel label("Animation", makeSpriteSheet());
	label.setGeometry(spk::Rect2D(0, 0, 32, 32));
	label.setDepth(1.5f);
	auto unitBefore = label.renderUnit();

	label.setDepth(1.5f);

	EXPECT_EQ(label.depth(), 1.5f);
}

TEST(AnimationLabelTest, SetDepthDifferentValueUpdates)
{
	spk::AnimationLabel label("Animation", makeSpriteSheet());
	label.setGeometry(spk::Rect2D(0, 0, 32, 32));

	label.setDepth(3.0f);

	EXPECT_FLOAT_EQ(label.depth(), 3.0f);
}

TEST(AnimationLabelVisualTest, RendersFirstFrame)
{
	const spk::Rect2D captureRect(0, 0, 96, 96);

	spk::AnimationLabel label("Anim", makeSpriteSheet());

	const sparkle_test::ImageComparisonResult result = spk::test::compareSnapshot(label, "AnimationLabelVisual", "frame_0", captureRect);

	EXPECT_TRUE(result.matches);
}

TEST(AnimationLabelVisualTest, RendersSecondFrame)
{
	const spk::Rect2D captureRect(0, 0, 96, 96);

	spk::AnimationLabel label("Anim", makeSpriteSheet());
	label.setLoopSpeed(spk::Duration(0.0L, spk::TimeUnit::Millisecond));
	spk::UpdateTick tick{};
	label.update(tick);

	const sparkle_test::ImageComparisonResult result = spk::test::compareSnapshot(label, "AnimationLabelVisual", "frame_1", captureRect);

	EXPECT_TRUE(result.matches);
}
