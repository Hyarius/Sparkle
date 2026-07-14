#pragma once

#include "core/definition_fields.hpp"
#include "core/json.hpp"
#include "core/registry.hpp"
#include "world/prefab_definition.hpp"

#include "structures/math/spk_vector3.hpp"
#include "structures/voxel/spk_voxel_enums.hpp"

#include <string>

namespace pg
{
	// A reusable handcrafted arena: the geometry a Gym or a Special encounter fights in, as opposed
	// to the live world a Wild or Trainer battle overlays.
	//
	// It is a definition, so it holds authored geometry and deployment and nothing else: no
	// occupancy, no navigation graph, no units, no runtime state. Step 05 materialises it - into an
	// owned grid, at the identity transform this file validates - and only then computes
	// standability and deployment capacity.
	struct HandcraftedBattleBoardDefinition
	{
		std::string id;
		// A translation key, not a sentence (see resources/data/locales).
		std::string displayNameKey;
		spk::Vector3Int size;
		// Resolved through the ordinary prefab registry: a battle board introduces no second voxel,
		// palette or prefab parser.
		std::string geometryPrefabId;
		// The side the player's team walks in from; the enemy deploys from the opposite edge.
		spk::VoxelOrientation playerApproach = spk::VoxelOrientation::PositiveZ;
		int deploymentDepth = 0;
		DefinitionSource source;
	};

	inline constexpr int HandcraftedBattleBoardSchemaVersion = 1;
	inline constexpr int MinimumBattleBoardSide = 5;
	inline constexpr int MaximumBattleBoardSide = 31;
	inline constexpr int MinimumBattleBoardHeight = 3;
	inline constexpr int MaximumBattleBoardHeight = 64;

	// The board extent along the approach axis: X for an east/west approach, Z for a north/south
	// one. Deployment eats deploymentDepth rows at each end of it, which is why 2 * depth has to
	// fit inside it.
	[[nodiscard]] int approachAxisExtent(const HandcraftedBattleBoardDefinition &p_board) noexcept;
	// The extent across it - the width of a deployment row.
	[[nodiscard]] int deploymentRowWidth(const HandcraftedBattleBoardDefinition &p_board) noexcept;

	// The board-local cells one side deploys in, inclusive corners. The player walks in along the
	// approach direction, so a positiveZ approach puts the player on the low-z edge and the enemy
	// on the high-z one; a negativeZ approach mirrors both.
	struct BoardDeploymentStrip
	{
		int minX = 0;
		int maxX = 0;
		int minZ = 0;
		int maxZ = 0;

		[[nodiscard]] bool contains(int p_x, int p_z) const noexcept;
	};

	[[nodiscard]] BoardDeploymentStrip enemyDeploymentStrip(const HandcraftedBattleBoardDefinition &p_board) noexcept;
	[[nodiscard]] BoardDeploymentStrip playerDeploymentStrip(const HandcraftedBattleBoardDefinition &p_board) noexcept;

	// Leaves the id empty: the registry loader assigns the validated filename stem. p_prefabs is
	// read for validation only - the definition stores the prefab id, never a reference into a
	// registry that the load transaction may still discard.
	//
	// The prefab is checked at the exact transform step 05 will stamp it with: identity, meaning
	// PositiveZ, no flip, and the prefab's own pivot as the destination, so an applied voxel lands
	// on its authored position. Every listed voxel - including a cell authored empty to carve the
	// arena open - must then fall inside [0, size). One cell outside rejects the board rather than
	// being silently clipped away by Prefab::applyTo.
	[[nodiscard]] HandcraftedBattleBoardDefinition parseHandcraftedBattleBoardDefinition(
		JsonReader &p_reader,
		const Registry<PrefabDefinition> &p_prefabs);
}
