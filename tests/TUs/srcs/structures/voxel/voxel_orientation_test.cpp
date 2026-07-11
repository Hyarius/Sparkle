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

TEST(VoxelOrientation, TransformsNormalizedVoxelPositionsForEveryOrientationAndFlip)
{
	using O = spk::VoxelOrientation;
	using F = spk::VoxelFlip;
	const spk::Vector3 position{0.2f, 0.3f, 0.4f};
	constexpr std::array orientations = {O::PositiveZ, O::PositiveX, O::NegativeZ, O::NegativeX};
	const std::array expected = {
		spk::Vector3{0.2f, 0.3f, 0.4f},
		spk::Vector3{0.4f, 0.3f, 0.8f},
		spk::Vector3{0.8f, 0.3f, 0.6f},
		spk::Vector3{0.6f, 0.3f, 0.2f}};

	for (std::size_t index = 0; index < orientations.size(); ++index)
	{
		EXPECT_EQ(spk::transformVoxelPosition(position, orientations[index], F::PositiveY), expected[index]);
		spk::Vector3 flipped = expected[index];
		flipped.y = 1.0f - flipped.y;
		EXPECT_EQ(spk::transformVoxelPosition(position, orientations[index], F::NegativeY), flipped);
	}
}

TEST(VoxelOrientation, MapsWorldPlanesToLocalForEveryOrientationAndFlip)
{
	using O = spk::VoxelOrientation;
	using P = spk::VoxelAxisPlane;
	using F = spk::VoxelFlip;

	constexpr std::array worldPlanes = {P::PositiveX, P::NegativeX, P::PositiveY, P::NegativeY, P::PositiveZ, P::NegativeZ};
	constexpr std::array orientations = {O::PositiveZ, O::PositiveX, O::NegativeZ, O::NegativeX};
	constexpr std::array positiveYMappings = {
		std::array{P::PositiveX, P::NegativeX, P::PositiveY, P::NegativeY, P::PositiveZ, P::NegativeZ},
		std::array{P::PositiveZ, P::NegativeZ, P::PositiveY, P::NegativeY, P::NegativeX, P::PositiveX},
		std::array{P::NegativeX, P::PositiveX, P::PositiveY, P::NegativeY, P::NegativeZ, P::PositiveZ},
		std::array{P::NegativeZ, P::PositiveZ, P::PositiveY, P::NegativeY, P::PositiveX, P::NegativeX}};

	for (std::size_t orientationIndex = 0; orientationIndex < orientations.size(); ++orientationIndex)
	{
		for (std::size_t planeIndex = 0; planeIndex < worldPlanes.size(); ++planeIndex)
		{
			EXPECT_EQ(
				spk::mapWorldPlaneToLocal(worldPlanes[planeIndex], orientations[orientationIndex], F::PositiveY),
				positiveYMappings[orientationIndex][planeIndex]);

			P flippedMapping = positiveYMappings[orientationIndex][planeIndex];
			if (flippedMapping == P::PositiveY)
			{
				flippedMapping = P::NegativeY;
			}
			else if (flippedMapping == P::NegativeY)
			{
				flippedMapping = P::PositiveY;
			}
			EXPECT_EQ(
				spk::mapWorldPlaneToLocal(worldPlanes[planeIndex], orientations[orientationIndex], F::NegativeY),
				flippedMapping);
		}
	}

	EXPECT_THROW((void)spk::mapWorldPlaneToLocal(P::Count, O::PositiveZ, F::PositiveY), std::invalid_argument);
}
