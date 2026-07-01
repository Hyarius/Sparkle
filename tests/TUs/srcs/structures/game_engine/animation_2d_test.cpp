#include <gtest/gtest.h>

#include "structures/game_engine/spk_animation_2d.hpp"

TEST(AnimationController2DTest, StartsWithoutCurrentAnimationAndStopped)
{
	spk::AnimationController2D controller;

	EXPECT_FALSE(controller.isPlaying());
	EXPECT_FALSE(controller.hasCurrent());
	EXPECT_TRUE(controller.currentName().empty());
	EXPECT_EQ(controller.currentAnimation(), nullptr);
	EXPECT_DOUBLE_EQ(controller.elapsedSeconds(), 0.0);
}

TEST(AnimationController2DTest, AddAndPlaySelectAnimation)
{
	spk::AnimationController2D controller;
	spk::Animation2D animation;
	animation.frames = {{0u, 0u}, {1u, 0u}};
	controller.addAnimation(L"walk", animation);

	EXPECT_TRUE(controller.hasAnimation(L"walk"));
	EXPECT_FALSE(controller.hasAnimation(L"idle"));

	controller.play(L"walk");

	EXPECT_TRUE(controller.isPlaying());
	EXPECT_TRUE(controller.hasCurrent());
	EXPECT_EQ(controller.currentName(), L"walk");
	ASSERT_NE(controller.currentAnimation(), nullptr);
	EXPECT_EQ(controller.currentAnimation()->frames, animation.frames);
}

TEST(AnimationController2DTest, PlayingUnknownAnimationDoesNotChangeState)
{
	spk::AnimationController2D controller;

	controller.play(L"missing");

	EXPECT_FALSE(controller.isPlaying());
	EXPECT_FALSE(controller.hasCurrent());
	EXPECT_EQ(controller.currentAnimation(), nullptr);
}

TEST(AnimationController2DTest, StopKeepsSelectionAndElapsedTime)
{
	spk::AnimationController2D controller;
	controller.addAnimation(L"walk", spk::Animation2D{});
	controller.play(L"walk");
	controller.addElapsed(1.25);

	controller.stop();

	EXPECT_FALSE(controller.isPlaying());
	EXPECT_TRUE(controller.hasCurrent());
	EXPECT_EQ(controller.currentName(), L"walk");
	EXPECT_DOUBLE_EQ(controller.elapsedSeconds(), 1.25);
}

TEST(AnimationController2DTest, ReplayingSameAnimationPreservesElapsedTime)
{
	spk::AnimationController2D controller;
	controller.addAnimation(L"walk", spk::Animation2D{});
	controller.play(L"walk");
	controller.addElapsed(2.5);
	controller.stop();

	controller.play(L"walk");

	EXPECT_TRUE(controller.isPlaying());
	EXPECT_DOUBLE_EQ(controller.elapsedSeconds(), 2.5);
}

TEST(AnimationController2DTest, SwitchingAnimationResetsElapsedTime)
{
	spk::AnimationController2D controller;
	controller.addAnimation(L"walk", spk::Animation2D{});
	controller.addAnimation(L"idle", spk::Animation2D{});
	controller.play(L"walk");
	controller.addElapsed(2.5);

	controller.play(L"idle");

	EXPECT_TRUE(controller.isPlaying());
	EXPECT_EQ(controller.currentName(), L"idle");
	EXPECT_DOUBLE_EQ(controller.elapsedSeconds(), 0.0);
}

TEST(AnimationController2DTest, ReplacingSelectedAnimationUpdatesCurrentValue)
{
	spk::AnimationController2D controller;
	spk::Animation2D first;
	first.loop = true;
	spk::Animation2D replacement;
	replacement.loop = false;
	controller.addAnimation(L"action", first);
	controller.play(L"action");

	controller.addAnimation(L"action", replacement);

	ASSERT_NE(controller.currentAnimation(), nullptr);
	EXPECT_FALSE(controller.currentAnimation()->loop);
}
