#include <gtest/gtest.h>

#include "structures/voxel/spk_voxel_orientation.hpp"

#include <array>

namespace
{
	constexpr std::array<spk::VoxelOrientation, 4> orientationsInTurnOrder = {
		spk::VoxelOrientation::PositiveZ,
		spk::VoxelOrientation::PositiveX,
		spk::VoxelOrientation::NegativeZ,
		spk::VoxelOrientation::NegativeX};
}

TEST(VoxelOrientation, QuarterTurnsRoundTrip)
{
	for (int turns = 0; turns < 4; ++turns)
	{
		EXPECT_EQ(spk::orientationFromQuarterTurns(turns), orientationsInTurnOrder[turns]);
		EXPECT_EQ(spk::quarterTurnsOf(orientationsInTurnOrder[turns]), turns);
	}
}

TEST(VoxelOrientation, TurnCountsNormalizeIncludingNegatives)
{
	EXPECT_EQ(spk::orientationFromQuarterTurns(4), spk::VoxelOrientation::PositiveZ);
	EXPECT_EQ(spk::orientationFromQuarterTurns(7), spk::VoxelOrientation::NegativeX);
	EXPECT_EQ(spk::orientationFromQuarterTurns(-1), spk::VoxelOrientation::NegativeX);
	EXPECT_EQ(spk::orientationFromQuarterTurns(-4), spk::VoxelOrientation::PositiveZ);
	EXPECT_EQ(spk::orientationFromQuarterTurns(-6), spk::VoxelOrientation::NegativeZ);
}

TEST(VoxelOrientation, ComposeOrientationsAddsTurns)
{
	for (const spk::VoxelOrientation first : orientationsInTurnOrder)
	{
		// PositiveZ is the identity on both sides.
		EXPECT_EQ(spk::composeOrientations(first, spk::VoxelOrientation::PositiveZ), first);
		EXPECT_EQ(spk::composeOrientations(spk::VoxelOrientation::PositiveZ, first), first);
		for (const spk::VoxelOrientation second : orientationsInTurnOrder)
		{
			EXPECT_EQ(
				spk::composeOrientations(first, second),
				spk::orientationFromQuarterTurns(spk::quarterTurnsOf(first) + spk::quarterTurnsOf(second)));
			EXPECT_EQ(spk::composeOrientations(first, second), spk::composeOrientations(second, first));
		}
	}
}

TEST(VoxelOrientation, ComposeFlipsCancelInPairs)
{
	EXPECT_EQ(spk::composeFlips(spk::VoxelFlip::PositiveY, spk::VoxelFlip::PositiveY), spk::VoxelFlip::PositiveY);
	EXPECT_EQ(spk::composeFlips(spk::VoxelFlip::PositiveY, spk::VoxelFlip::NegativeY), spk::VoxelFlip::NegativeY);
	EXPECT_EQ(spk::composeFlips(spk::VoxelFlip::NegativeY, spk::VoxelFlip::PositiveY), spk::VoxelFlip::NegativeY);
	EXPECT_EQ(spk::composeFlips(spk::VoxelFlip::NegativeY, spk::VoxelFlip::NegativeY), spk::VoxelFlip::PositiveY);
}

TEST(VoxelOrientation, RotateQuarterTurnsMatchesTheAxisConvention)
{
	// One quarter turn sends local +Z to world +X (the orientation names give the
	// world direction of the shape's local +Z axis).
	const spk::Vector3Int localForward{0, 0, 1};
	EXPECT_EQ(spk::rotateQuarterTurns(localForward, 0), spk::Vector3Int(0, 0, 1));
	EXPECT_EQ(spk::rotateQuarterTurns(localForward, 1), spk::Vector3Int(1, 0, 0));
	EXPECT_EQ(spk::rotateQuarterTurns(localForward, 2), spk::Vector3Int(0, 0, -1));
	EXPECT_EQ(spk::rotateQuarterTurns(localForward, 3), spk::Vector3Int(-1, 0, 0));
}

TEST(VoxelOrientation, RotateQuarterTurnsPreservesYAndNormalizesTurns)
{
	const spk::Vector3Int position{3, -2, 5};
	for (int turns = -8; turns <= 8; ++turns)
	{
		const spk::Vector3Int rotated = spk::rotateQuarterTurns(position, turns);
		EXPECT_EQ(rotated.y, position.y);
		EXPECT_EQ(rotated, spk::rotateQuarterTurns(position, ((turns % 4) + 4) % 4));
	}
	// Four quarter turns compose to the identity.
	spk::Vector3Int cursor = position;
	for (int step = 0; step < 4; ++step)
	{
		cursor = spk::rotateQuarterTurns(cursor, 1);
	}
	EXPECT_EQ(cursor, position);
}
