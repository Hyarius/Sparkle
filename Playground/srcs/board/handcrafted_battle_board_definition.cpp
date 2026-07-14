#include "board/handcrafted_battle_board_definition.hpp"

#include "core/content_id.hpp"

#include "structures/voxel/spk_prefab.hpp"

#include <array>
#include <cstdint>
#include <map>
#include <string>

namespace pg
{
	namespace
	{
		// The four horizontal approaches. A vertical one would have no deployment strip, so the
		// board schema does not accept the vocabulary that would let an author write one.
		[[nodiscard]] const std::map<std::string, spk::VoxelOrientation> &approachNames()
		{
			static const std::map<std::string, spk::VoxelOrientation> names{
				{"negativeX", spk::VoxelOrientation::NegativeX},
				{"negativeZ", spk::VoxelOrientation::NegativeZ},
				{"positiveX", spk::VoxelOrientation::PositiveX},
				{"positiveZ", spk::VoxelOrientation::PositiveZ}};
			return names;
		}

		[[nodiscard]] spk::Vector3Int requireSize(const JsonReader &p_reader)
		{
			const std::array<int, 3> authored = p_reader.require<std::array<int, 3>>("size");
			const spk::Vector3Int size{authored[0], authored[1], authored[2]};

			const auto requireSide = [&p_reader](int p_value, const char *p_axis) {
				if (p_value < MinimumBattleBoardSide || p_value > MaximumBattleBoardSide)
				{
					throw JsonError(
						p_reader.file(),
						p_reader.pathFor("size"),
						std::string(p_axis) + " is an integer in [" + std::to_string(MinimumBattleBoardSide) + ", " +
							std::to_string(MaximumBattleBoardSide) + "], got " + std::to_string(p_value));
				}
				// Odd, so the board has a single centre column and row - which is what a centre-out
				// deployment order and a symmetric arena both assume.
				if (p_value % 2 == 0)
				{
					throw JsonError(
						p_reader.file(),
						p_reader.pathFor("size"),
						std::string(p_axis) + " is odd, so the board has one centre cell; got " + std::to_string(p_value));
				}
			};

			requireSide(size.x, "size.x");
			requireSide(size.z, "size.z");
			if (size.y < MinimumBattleBoardHeight || size.y > MaximumBattleBoardHeight)
			{
				throw JsonError(
					p_reader.file(),
					p_reader.pathFor("size"),
					"size.y is an integer in [" + std::to_string(MinimumBattleBoardHeight) + ", " +
						std::to_string(MaximumBattleBoardHeight) + "], got " + std::to_string(size.y));
			}

			// The v1 limits keep this far below any overflow, and it is computed wide anyway: the
			// cell count is what step 05 allocates a grid from, and a definition never hands an
			// allocator a product nobody checked.
			const std::int64_t volume = static_cast<std::int64_t>(size.x) * static_cast<std::int64_t>(size.y) *
										static_cast<std::int64_t>(size.z);
			constexpr std::int64_t MaximumVolume = static_cast<std::int64_t>(MaximumBattleBoardSide) *
												   static_cast<std::int64_t>(MaximumBattleBoardSide) *
												   static_cast<std::int64_t>(MaximumBattleBoardHeight);
			if (volume <= 0 || volume > MaximumVolume)
			{
				throw JsonError(
					p_reader.file(),
					p_reader.pathFor("size"),
					"the board holds " + std::to_string(volume) + " cells, over the " + std::to_string(MaximumVolume) +
						" a v1 arena may allocate");
			}
			return size;
		}

		void requirePrefabInsideBoard(
			const JsonReader &p_reader,
			const HandcraftedBattleBoardDefinition &p_board,
			const PrefabDefinition &p_prefab)
		{
			// Identity means PositiveZ, no flip, and the prefab's own pivot as the destination, so
			// an applied voxel lands exactly on its authored position. Checking the authored
			// positions is therefore checking the applied ones - and it is what step 05 stamps.
			for (const spk::Prefab::Voxel &voxel : p_prefab.prefab.voxels())
			{
				const spk::Vector3Int &position = voxel.position;
				const bool inside = position.x >= 0 && position.x < p_board.size.x && position.y >= 0 &&
									position.y < p_board.size.y && position.z >= 0 && position.z < p_board.size.z;
				if (!inside)
				{
					// Prefab::applyTo would quietly clip this cell, and an arena silently missing a
					// wall is worse than one that refuses to load.
					throw JsonError(
						p_reader.file(),
						p_reader.pathFor("geometryPrefab"),
						"prefab '" + p_prefab.id + "' places a voxel at " + position.toString() +
							", outside the board's [0, " + p_board.size.toString() + ") box");
				}
			}
		}
	}

	int approachAxisExtent(const HandcraftedBattleBoardDefinition &p_board) noexcept
	{
		switch (p_board.playerApproach)
		{
		case spk::VoxelOrientation::PositiveX:
		case spk::VoxelOrientation::NegativeX:
			return p_board.size.x;
		case spk::VoxelOrientation::PositiveZ:
		case spk::VoxelOrientation::NegativeZ:
			return p_board.size.z;
		}
		return 0;
	}

	int deploymentRowWidth(const HandcraftedBattleBoardDefinition &p_board) noexcept
	{
		switch (p_board.playerApproach)
		{
		case spk::VoxelOrientation::PositiveX:
		case spk::VoxelOrientation::NegativeX:
			return p_board.size.z;
		case spk::VoxelOrientation::PositiveZ:
		case spk::VoxelOrientation::NegativeZ:
			return p_board.size.x;
		}
		return 0;
	}

	bool BoardDeploymentStrip::contains(int p_x, int p_z) const noexcept
	{
		return p_x >= minX && p_x <= maxX && p_z >= minZ && p_z <= maxZ;
	}

	BoardDeploymentStrip enemyDeploymentStrip(const HandcraftedBattleBoardDefinition &p_board) noexcept
	{
		BoardDeploymentStrip strip{.minX = 0, .maxX = p_board.size.x - 1, .minZ = 0, .maxZ = p_board.size.z - 1};

		// The player walks in along the approach direction, so it lands on the edge that direction
		// points away from and the enemy waits on the far one.
		switch (p_board.playerApproach)
		{
		case spk::VoxelOrientation::PositiveX:
			strip.minX = p_board.size.x - p_board.deploymentDepth;
			break;
		case spk::VoxelOrientation::NegativeX:
			strip.maxX = p_board.deploymentDepth - 1;
			break;
		case spk::VoxelOrientation::PositiveZ:
			strip.minZ = p_board.size.z - p_board.deploymentDepth;
			break;
		case spk::VoxelOrientation::NegativeZ:
			strip.maxZ = p_board.deploymentDepth - 1;
			break;
		}
		return strip;
	}

	BoardDeploymentStrip playerDeploymentStrip(const HandcraftedBattleBoardDefinition &p_board) noexcept
	{
		BoardDeploymentStrip strip{.minX = 0, .maxX = p_board.size.x - 1, .minZ = 0, .maxZ = p_board.size.z - 1};

		switch (p_board.playerApproach)
		{
		case spk::VoxelOrientation::PositiveX:
			strip.maxX = p_board.deploymentDepth - 1;
			break;
		case spk::VoxelOrientation::NegativeX:
			strip.minX = p_board.size.x - p_board.deploymentDepth;
			break;
		case spk::VoxelOrientation::PositiveZ:
			strip.maxZ = p_board.deploymentDepth - 1;
			break;
		case spk::VoxelOrientation::NegativeZ:
			strip.minZ = p_board.size.z - p_board.deploymentDepth;
			break;
		}
		return strip;
	}

	HandcraftedBattleBoardDefinition parseHandcraftedBattleBoardDefinition(
		JsonReader &p_reader,
		const Registry<PrefabDefinition> &p_prefabs)
	{
		p_reader.forbidUnknown({"version", "displayNameKey", "size", "geometryPrefab", "playerApproach", "deploymentDepth"});
		requireVersion(p_reader, HandcraftedBattleBoardSchemaVersion);

		HandcraftedBattleBoardDefinition result;
		result.displayNameKey = requireDisplayNameKey(p_reader);
		result.size = requireSize(p_reader);
		result.geometryPrefabId = p_reader.require<std::string>("geometryPrefab");
		requireContentId(
			result.geometryPrefabId,
			p_reader.file(),
			p_reader.pathFor("geometryPrefab"),
			"prefab id");
		result.playerApproach = p_reader.requireEnum<spk::VoxelOrientation>("playerApproach", approachNames());
		result.source = DefinitionSource{p_reader.file(), p_reader.path()};

		const int extent = approachAxisExtent(result);
		// Checked as a sum rather than as a product so the comparison cannot overflow on the way to
		// rejecting an absurd depth.
		result.deploymentDepth =
			static_cast<int>(requireIntegerInRange(p_reader, "deploymentDepth", 1, extent));
		if (result.deploymentDepth > extent - result.deploymentDepth)
		{
			throw JsonError(
				p_reader.file(),
				p_reader.pathFor("deploymentDepth"),
				"both deployment strips are " + std::to_string(result.deploymentDepth) +
					" rows deep and the approach axis is only " + std::to_string(extent) + " rows long");
		}

		const PrefabDefinition *prefab = p_prefabs.tryGet(result.geometryPrefabId);
		if (prefab == nullptr)
		{
			throw JsonError(
				p_reader.file(),
				p_reader.pathFor("geometryPrefab"),
				"unknown prefab '" + result.geometryPrefabId + "'");
		}
		requirePrefabInsideBoard(p_reader, result, *prefab);
		return result;
	}
}
