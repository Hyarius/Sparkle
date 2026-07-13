#include "world/generator/world_plan_generator.hpp"
#include "structures/voxel/spk_voxel_orientation.hpp"

#include <algorithm>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

// Composed interiors: buildings with an interiorId grow a seeded set of rooms in
// a reserved void slot east of the world and pair their door with the entry/exit
// pads through one-way portals.
namespace pg::worldgen
{
	// Room prefabs connect through anchors named "connector:<axis>", placed on
	// the wall block they can open through, one cell above the room floor.
	std::optional<spk::Vector3Int> Generator::connectorDirection(const std::string &p_name)
	{
		if (!p_name.starts_with("connector:"))
		{
			return std::nullopt;
		}
		const std::string_view axis = std::string_view(p_name).substr(10);
		if (axis == "+x")
		{
			return spk::Vector3Int{1, 0, 0};
		}
		if (axis == "-x")
		{
			return spk::Vector3Int{-1, 0, 0};
		}
		if (axis == "+z")
		{
			return spk::Vector3Int{0, 0, 1};
		}
		if (axis == "-z")
		{
			return spk::Vector3Int{0, 0, -1};
		}
		return std::nullopt;
	}

	// Composes the interior linked by a just-placed building: grows a seeded set
	// of rooms from the entry room inside a reserved void slot east of the world,
	// carves the doorways between connected rooms, and pairs the building's door
	// with the entry/exit pads through two one-way portals.
	void Generator::composeInterior(const PrefabPlacement &p_buildingPlacement)
	{
		const PrefabDefinition *building = prefabs.tryGet(p_buildingPlacement.prefabId);
		if (building == nullptr || building->interiorId.empty())
		{
			return;
		}
		const InteriorDefinition *interior = interiors.tryGet(building->interiorId);
		const PrefabAnchor *door = building->tryAnchor("door");
		const PrefabDefinition *entryRoom =
			interior == nullptr ? nullptr : prefabs.tryGet(interior->entryPrefabId);
		const PrefabAnchor *entryPad = entryRoom == nullptr ? nullptr : entryRoom->tryAnchor("entry");
		const PrefabAnchor *exitPad = entryRoom == nullptr ? nullptr : entryRoom->tryAnchor("exit");
		const std::optional<ResolvedPlacementBox> buildingBox = resolveBox(p_buildingPlacement);
		if (interior == nullptr || door == nullptr || entryPad == nullptr || exitPad == nullptr ||
			!buildingBox.has_value())
		{
			plan.stats.warnings.push_back(
				"interior '" + building->interiorId + "' of prefab '" + building->id + "' could not be composed");
			return;
		}

		// The door cell is the floor block inside the doorway (anchored at local
		// y = -1); the return portal drops the player one cell outside it, on the
		// same flattened plateau. Resolve the anchor and outward vector through the
		// committed building rotation; town blueprints may use any quarter turn.
		const spk::Vector3Int buildingPivot = building->prefab.pivot();
		const int buildingTurns=spk::quarterTurnsOf(p_buildingPlacement.orientation);
		const spk::Vector3Int doorWorld=buildingBox->destination+spk::rotateQuarterTurns(door->position-buildingPivot,buildingTurns);
		const spk::Vector3Int outward=spk::rotateQuarterTurns({0,0,-1},buildingTurns);

		// One square void slot per interior, along the +Z edge of the band. Rooms
		// share the level-0 ground height so walk heights match the overworld.
		const int slotIndex = interiorSlotsUsed++;
		const int slotSide = cfg.interiorSlotBlocks;
		const int groundY = plan.surfaceY(0) + 1;
		const Claim slotBounds{
			.min = {plan.interiorRegionMinX(), 0, plan.worldOffset() + slotIndex * slotSide},
			.max = {plan.interiorRegionMinX() + slotSide - 1, 63, plan.worldOffset() + (slotIndex + 1) * slotSide - 1}};
		Rng rng = rngFor("interior:" + std::to_string(slotIndex));

		struct OpenConnector
		{
			spk::Vector3Int at{}; // world wall cell the connector opens through
			spk::Vector3Int direction{};
		};
		std::vector<Claim> roomBoxes;
		std::vector<OpenConnector> open;

		const auto placeRoom = [&](const PrefabDefinition &p_room, const spk::Vector3Int &p_destination) {
			plan.placements.push_back(
				{.prefabId = p_room.id,
				 .anchor = p_destination,
				 .orientation = spk::VoxelOrientation::PositiveZ,
				 .foundation = false,
				 .anchorToPivot = true});
			const spk::Vector3Int pivot = p_room.prefab.pivot();
			roomBoxes.push_back(
				{.min = p_destination + p_room.prefab.minBounds() - pivot,
				 .max = p_destination + p_room.prefab.maxBounds() - pivot});
			hardClaims.push_back(roomBoxes.back());
			for (const PrefabAnchor &anchor : p_room.prefab.anchors())
			{
				if (const std::optional<spk::Vector3Int> direction = connectorDirection(anchor.name))
				{
					open.push_back({.at = p_destination + anchor.position - pivot, .direction = *direction});
				}
			}
			++plan.stats.interiorRoomPlacements;
		};

		const spk::Vector3Int entryDestination{
			slotBounds.min.x + slotSide / 2, groundY, slotBounds.min.z + slotSide / 2};
		placeRoom(*entryRoom, entryDestination);

		const int extraRange = interior->maxRooms - interior->minRooms;
		const int extraTarget = interior->minRooms + (extraRange > 0 ? rng.below(extraRange + 1) : 0);
		int extraPlaced = 0;
		for (int attempt = 0; attempt < 64 && extraPlaced < extraTarget && !open.empty(); ++attempt)
		{
			const OpenConnector connector = open[rng.below(static_cast<int>(open.size()))];
			const std::string &roomPrefabId = interior->rooms.pick(rng.uniform());
			const PrefabDefinition *candidate = prefabs.tryGet(roomPrefabId);
			if (candidate == nullptr)
			{
				continue;
			}
			const spk::Vector3Int inwardDirection{
				-connector.direction.x, -connector.direction.y, -connector.direction.z};
			std::vector<const PrefabAnchor *> mates;
			for (const PrefabAnchor &anchor : candidate->prefab.anchors())
			{
				const std::optional<spk::Vector3Int> direction = connectorDirection(anchor.name);
				if (direction.has_value() && *direction == inwardDirection)
				{
					mates.push_back(&anchor);
				}
			}
			if (mates.empty())
			{
				continue;
			}
			const PrefabAnchor *mate = mates[rng.below(static_cast<int>(mates.size()))];
			const spk::Vector3Int mateWall = connector.at + connector.direction;
			const spk::Vector3Int candidatePivot = candidate->prefab.pivot();
			const spk::Vector3Int destination = mateWall - (mate->position - candidatePivot);
			const Claim box{
				.min = destination + candidate->prefab.minBounds() - candidatePivot,
				.max = destination + candidate->prefab.maxBounds() - candidatePivot};
			const bool insideSlot = box.min.x >= slotBounds.min.x && box.max.x <= slotBounds.max.x &&
									box.min.z >= slotBounds.min.z && box.max.z <= slotBounds.max.z;
			const bool collides = std::ranges::any_of(roomBoxes, [&](const Claim &p_placed) {
				return claimsOverlap(box, p_placed);
			});
			if (!insideSlot || collides)
			{
				continue;
			}

			placeRoom(*candidate, destination);
			// The rooms abut wall against wall: carve the two facing wall blocks
			// (body height, two blocks tall) so the shared doorway opens up.
			plan.placements.push_back(
				{.prefabId = connector.direction.x != 0 ? "interior-doorway-x" : "interior-doorway-z",
				 .anchor = {std::min(connector.at.x, mateWall.x), connector.at.y, std::min(connector.at.z, mateWall.z)},
				 .orientation = spk::VoxelOrientation::PositiveZ,
				 .foundation = false,
				 .anchorToPivot = true});
			// Both halves of the joint are connected now; neither may host another room.
			std::erase_if(open, [&](const OpenConnector &p_open) {
				return (p_open.at == connector.at && p_open.direction == connector.direction) ||
					   (p_open.at == mateWall && p_open.direction == inwardDirection);
			});
			++extraPlaced;
		}

		const spk::Vector3Int entryPivot = entryRoom->prefab.pivot();
		plan.portals.push_back({.from = doorWorld, .to = entryDestination + entryPad->position - entryPivot});
		plan.portals.push_back({.from = entryDestination + exitPad->position - entryPivot, .to = doorWorld + outward});
		++plan.stats.interiorCount;
	}
}
