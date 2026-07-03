#include "logics/tactical_camera_logic.hpp"

#include <gtest/gtest.h>

#include <cmath>

namespace
{
	using Pose = pg::TacticalCameraLogic::Pose;
}

TEST(TacticalCameraLogic, BlendPoseIsFromAtZeroTargetAtOneMidpointAtHalf)
{
	const Pose from{{0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 0.0f}};
	const Pose to{{10.0f, 20.0f, 30.0f}, {2.0f, 4.0f, 6.0f}};

	const Pose atZero = pg::TacticalCameraLogic::blendPose(from, to, 0.0f);
	EXPECT_FLOAT_EQ(atZero.position.x, 0.0f);
	EXPECT_FLOAT_EQ(atZero.position.z, 0.0f);

	const Pose atOne = pg::TacticalCameraLogic::blendPose(from, to, 1.0f);
	EXPECT_FLOAT_EQ(atOne.position.x, 10.0f);
	EXPECT_FLOAT_EQ(atOne.position.z, 30.0f);
	EXPECT_FLOAT_EQ(atOne.target.y, 4.0f);

	// smoothstep(0.5) == 0.5, so half-blend is the exact midpoint.
	const Pose atHalf = pg::TacticalCameraLogic::blendPose(from, to, 0.5f);
	EXPECT_FLOAT_EQ(atHalf.position.x, 5.0f);
	EXPECT_FLOAT_EQ(atHalf.position.y, 10.0f);
	EXPECT_FLOAT_EQ(atHalf.position.z, 15.0f);
	EXPECT_FLOAT_EQ(atHalf.target.x, 1.0f);
}

TEST(TacticalCameraLogic, BlendPoseClampsOutOfRangeTime)
{
	const Pose from{{1.0f, 2.0f, 3.0f}, {0.0f, 0.0f, 0.0f}};
	const Pose to{{9.0f, 9.0f, 9.0f}, {1.0f, 1.0f, 1.0f}};

	const Pose below = pg::TacticalCameraLogic::blendPose(from, to, -1.0f);
	EXPECT_FLOAT_EQ(below.position.x, 1.0f);
	const Pose above = pg::TacticalCameraLogic::blendPose(from, to, 2.0f);
	EXPECT_FLOAT_EQ(above.position.z, 9.0f);
}

TEST(TacticalCameraLogic, FramePoseCentersOnTheBoardAndSitsBackAtPitch)
{
	const Pose pose = pg::TacticalCameraLogic::framePose({0, 0, 0}, {10, 10}, 0.0f);

	// Target is the board centre (anchor + half extent, lifted a touch off the ground).
	EXPECT_FLOAT_EQ(pose.target.x, 5.0f);
	EXPECT_FLOAT_EQ(pose.target.z, 5.0f);

	// Yaw 0 keeps the camera on the +Z side; the ~50 deg pitch lifts it above the board.
	EXPECT_FLOAT_EQ(pose.position.x, 5.0f);
	EXPECT_GT(pose.position.y, pose.target.y);
	EXPECT_GT(pose.position.z, pose.target.z);

	const float distance = std::sqrt(
		std::pow(pose.position.x - pose.target.x, 2.0f) +
		std::pow(pose.position.y - pose.target.y, 2.0f) +
		std::pow(pose.position.z - pose.target.z, 2.0f));
	EXPECT_NEAR(distance, 10.0f * 1.15f + 6.0f, 0.01f);
}
