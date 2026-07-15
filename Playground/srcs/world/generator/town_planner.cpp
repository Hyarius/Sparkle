#include "world/generator/town_planner.hpp"

#include "world/generator/world_plan_math.hpp"
#include "world/prefab_placement_math.hpp"

#include "structures/voxel/spk_voxel_orientation.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <map>
#include <queue>
#include <set>
#include <tuple>

namespace pg
{
	namespace
	{
		struct BuildingInstance
		{
			std::string id;
			TownBuildingRole role = TownBuildingRole::None;
			bool required = true;
			int spacing = 0;
			int priority = 0;
			std::string prefabId;
			const PrefabDefinition *prefab = nullptr;
		};

		struct RouteResult
		{
			int cost = 0;
			std::vector<TownColumn> path;
		};

		[[nodiscard]] bool isSettlement(PlanEntityKind p_kind)
		{
			return p_kind == PlanEntityKind::City || p_kind == PlanEntityKind::Gym || p_kind == PlanEntityKind::PortCity;
		}

		[[nodiscard]] TownCompositionKind compositionKindFor(PlanEntityKind p_kind)
		{
			return p_kind == PlanEntityKind::Gym ? TownCompositionKind::Gym : p_kind == PlanEntityKind::PortCity ? TownCompositionKind::Port
																												 : TownCompositionKind::City;
		}

		[[nodiscard]] TownBuildingRole roleFor(PlanEntityKind p_kind)
		{
			return p_kind == PlanEntityKind::Gym ? TownBuildingRole::Gym : p_kind == PlanEntityKind::PortCity ? TownBuildingRole::Port
																											  : TownBuildingRole::CreatureCenter;
		}

		[[nodiscard]] spk::Vector3Int outwardFor(int p_turns, spk::VoxelOrientation p_canonical)
		{
			spk::Vector3Int direction{};
			switch (p_canonical)
			{
			case spk::VoxelOrientation::PositiveX:
				direction = {1, 0, 0};
				break;
			case spk::VoxelOrientation::NegativeX:
				direction = {-1, 0, 0};
				break;
			case spk::VoxelOrientation::PositiveZ:
				direction = {0, 0, 1};
				break;
			case spk::VoxelOrientation::NegativeZ:
				direction = {0, 0, -1};
				break;
			}
			return spk::rotateQuarterTurns(direction, p_turns);
		}

		[[nodiscard]] PlanClaim placementClaim(const PrefabDefinition &p_prefab, const PrefabPlacement &p_placement)
		{
			const int turns = spk::quarterTurnsOf(p_placement.orientation);
			const spk::Vector3Int pivot = p_prefab.prefab.pivot();
			const spk::Vector3Int localMin = p_prefab.clearance ? p_prefab.clearance->min : p_prefab.prefab.minBounds();
			const spk::Vector3Int localMax = p_prefab.clearance ? p_prefab.clearance->max : p_prefab.prefab.maxBounds();
			const spk::Vector3Int a = spk::rotateQuarterTurns(localMin - pivot, turns);
			const spk::Vector3Int b = spk::rotateQuarterTurns(localMax - pivot, turns);
			return {.min = p_placement.anchor + spk::Vector3Int{std::min(a.x, b.x), std::min(a.y, b.y), std::min(a.z, b.z)}, .max = p_placement.anchor + spk::Vector3Int{std::max(a.x, b.x), std::max(a.y, b.y), std::max(a.z, b.z)}};
		}

		[[nodiscard]] bool overlaps(const PlanClaim &p_first, const PlanClaim &p_second)
		{
			return p_first.min.x <= p_second.max.x && p_first.max.x >= p_second.min.x &&
				   p_first.min.y <= p_second.max.y && p_first.max.y >= p_second.min.y &&
				   p_first.min.z <= p_second.max.z && p_first.max.z >= p_second.min.z;
		}

		[[nodiscard]] std::vector<TownColumn> columnsIn(const TownWorkspace &p_workspace, const PlanClaim &p_claim)
		{
			std::vector<TownColumn> result;
			for (int z = p_claim.min.z; z <= p_claim.max.z; ++z)
			{
				for (int x = p_claim.min.x; x <= p_claim.max.x; ++x)
				{
					try
					{
						result.push_back(p_workspace.townFromWorld({x, z}));
					} catch (const std::out_of_range &)
					{
						return {};
					}
				}
			}
			return result;
		}

		[[nodiscard]] std::vector<TownColumn> solidColumns(const TownWorkspace &p_workspace, const ResolvedPlacementBox &p_box)
		{
			return columnsIn(p_workspace, {.min = p_box.worldMin, .max = p_box.worldMin + p_box.extents - spk::Vector3Int{1, 1, 1}});
		}

		[[nodiscard]] const std::string *prefabForRole(const PlanTown &p_town, TownBuildingRole p_role, worldgen::Rng &p_rng)
		{
			switch (p_role)
			{
			case TownBuildingRole::CreatureCenter:
				return &p_town.creatureCenter;
			case TownBuildingRole::Shop:
				return &p_town.shop;
			case TownBuildingRole::Gym:
				return &p_town.gym;
			case TownBuildingRole::Port:
				return &p_town.port;
			case TownBuildingRole::Home:
				if (!p_town.homes.empty())
				{
					return &p_town.homes[static_cast<std::size_t>(p_rng.below(static_cast<int>(p_town.homes.size())))];
				}
				break;
			case TownBuildingRole::None:
				break;
			}
			return nullptr;
		}

		[[nodiscard]] std::optional<std::vector<BuildingInstance>> expandInstances(
			const TownComposition &p_composition,
			const PlanTown &p_biomeTown,
			const Registry<PrefabDefinition> &p_prefabs,
			std::uint64_t p_seed,
			std::size_t p_entityIndex,
			TownRejection &p_rejection)
		{
			std::vector<BuildingInstance> result;
			const std::string prefix = "town/" + std::to_string(p_entityIndex) + "/buildings/";
			for (const TownBuildingRequest &request : p_composition.buildings)
			{
				worldgen::Rng countRng(worldgen::deriveSeed(p_seed, prefix + request.id + "/count"));
				const int count = request.count.minimum + (request.count.maximum > request.count.minimum ? countRng.below(request.count.maximum - request.count.minimum + 1) : 0);
				for (int index = 0; index < count; ++index)
				{
					const std::string id = request.id + "/" + std::to_string(index);
					worldgen::Rng prefabRng(worldgen::deriveSeed(p_seed, prefix + id + "/prefab"));
					const std::string *prefabId = prefabForRole(p_biomeTown, request.role, prefabRng);
					const PrefabDefinition *prefab = prefabId == nullptr ? nullptr : p_prefabs.tryGet(*prefabId);
					if (prefab == nullptr || !prefab->entrance || prefab->tryAnchor(prefab->entrance->anchorName) == nullptr)
					{
						p_rejection = {.category = TownRejectCategory::MissingContent, .componentId = id, .message = "required town role has no prefab entrance contract"};
						return std::nullopt;
					}
					result.push_back({.id = id, .role = request.role, .required = request.required, .spacing = request.minimumSpacing, .priority = request.placementPriority, .prefabId = *prefabId, .prefab = prefab});
				}
			}
			return result;
		}

		[[nodiscard]] int clearanceArea(const BuildingInstance &p_instance)
		{
			const spk::Vector3Int low = p_instance.prefab->clearance ? p_instance.prefab->clearance->min : p_instance.prefab->prefab.minBounds();
			const spk::Vector3Int high = p_instance.prefab->clearance ? p_instance.prefab->clearance->max : p_instance.prefab->prefab.maxBounds();
			return (high.x - low.x + 1) * (high.z - low.z + 1);
		}

		void sortInstances(std::vector<BuildingInstance> &p_instances, std::uint64_t p_seed, std::size_t p_entityIndex)
		{
			std::sort(p_instances.begin(), p_instances.end(), [&](const BuildingInstance &p_left, const BuildingInstance &p_right) {
				const auto rank = [](const BuildingInstance &instance) {
					return std::tuple{instance.required ? 0 : 1, -instance.priority, -clearanceArea(instance)};
				};
				if (rank(p_left) != rank(p_right))
				{
					return rank(p_left) < rank(p_right);
				}
				const std::uint64_t leftKey = worldgen::deriveSeed(p_seed, "town/" + std::to_string(p_entityIndex) + "/order/" + p_left.id);
				const std::uint64_t rightKey = worldgen::deriveSeed(p_seed, "town/" + std::to_string(p_entityIndex) + "/order/" + p_right.id);
				return leftKey != rightKey ? leftKey < rightKey : p_left.id < p_right.id;
			});
		}

		[[nodiscard]] bool validSurface(const TownWorkspace &p_workspace, TownColumn p_column, int p_height)
		{
			return p_workspace.contains(p_column) && p_workspace.has(p_column, TownWorkspaceLayer::Land) &&
				   !p_workspace.has(p_column, TownWorkspaceLayer::TerrainBlocked) && !p_workspace.has(p_column, TownWorkspaceLayer::Stair) && p_workspace.surfaceHeight(p_column) == p_height;
		}

		// Town building footprints are deterministically terraced by PlanChunkProvider
		// at their selected door level. Requiring a prefab to sit inside one macro-cell
		// height rejects ordinary sites as soon as a 9-column building crosses an 8-column
		// cell boundary. Doors, approaches, and roads still use validSurface() and remain
		// exactly level; only the building foundation may cut/fill dry, clear land.
		[[nodiscard]] bool validBuildingSurface(const TownWorkspace &p_workspace, TownColumn p_column)
		{
			return p_workspace.contains(p_column) && p_workspace.has(p_column, TownWorkspaceLayer::Land) &&
				   !p_workspace.has(p_column, TownWorkspaceLayer::TerrainBlocked) && !p_workspace.has(p_column, TownWorkspaceLayer::Stair);
		}

		[[nodiscard]] std::optional<ResolvedTownEntrance> validateAndPlaceBuilding(
			TownWorkspace &p_workspace,
			const WorldPlan &p_plan,
			const BuildingInstance &p_instance,
			const PrefabPlacement &p_placement,
			std::vector<PlanClaim> &p_claims,
			TownRejection &p_rejection)
		{
			const PrefabEntrance &entrance = *p_instance.prefab->entrance;
			const PrefabAnchor *door = p_instance.prefab->tryAnchor(entrance.anchorName);
			const int turns = spk::quarterTurnsOf(p_placement.orientation);
			const ResolvedPlacementBox box = resolvePlacement(p_instance.prefab->prefab, p_placement);
			const spk::Vector3Int threshold = box.destination + spk::rotateQuarterTurns(door->position - p_instance.prefab->prefab.pivot(), turns);
			const spk::Vector3Int outward = outwardFor(turns, entrance.outwardFacing);
			const PlanClaim claim = placementClaim(*p_instance.prefab, p_placement);
			const std::vector<TownColumn> solids = solidColumns(p_workspace, box);
			const std::vector<TownColumn> clearance = columnsIn(p_workspace, claim);
			if (solids.empty() || clearance.empty())
			{
				p_rejection = {.category = TownRejectCategory::BuildingPlacement, .componentId = p_instance.id, .message = "building footprint leaves town boundary"};
				return std::nullopt;
			}
			const TownColumn thresholdColumn = p_workspace.townFromWorld({threshold.x, threshold.z});
			const int surface = p_workspace.surfaceHeight(thresholdColumn);
			if (threshold.y != surface)
			{
				p_rejection = {.category = TownRejectCategory::DoorClearance, .componentId = p_instance.id, .worldColumn = WorldColumn{threshold.x, threshold.z}, .message = "door threshold is not flush with terrain"};
				return std::nullopt;
			}
			for (TownColumn column : clearance)
			{
				if (!validBuildingSurface(p_workspace, column) || p_workspace.has(column, TownWorkspaceLayer::MainRoad) ||
					p_workspace.has(column, TownWorkspaceLayer::BuildingClearance) || p_workspace.has(column, TownWorkspaceLayer::BuildingSolid) || p_workspace.has(column, TownWorkspaceLayer::RoadsideReserved))
				{
					p_rejection = {.category = TownRejectCategory::BuildingPlacement, .componentId = p_instance.id, .worldColumn = p_workspace.worldFromTown(column), .message = "building clearance is not dry or clear"};
					return std::nullopt;
				}
			}
			for (const PlanClaim &other : p_claims)
			{
				if (overlaps(claim, other))
				{
					p_rejection = {.category = TownRejectCategory::BuildingPlacement, .componentId = p_instance.id, .message = "building claim overlaps another claim"};
					return std::nullopt;
				}
			}
			for (const PlanClaim &other : p_plan.townClaims)
			{
				if (overlaps(claim, other))
				{
					p_rejection = {.category = TownRejectCategory::BuildingPlacement, .componentId = p_instance.id, .message = "building claim overlaps committed town infrastructure"};
					return std::nullopt;
				}
			}

			std::set<TownColumn> approach;
			int longestApproach = 0;
			for (int z = entrance.clearApproach.min.z; z <= entrance.clearApproach.max.z; ++z)
			{
				for (int y = entrance.clearApproach.min.y; y <= entrance.clearApproach.max.y; ++y)
				{
					for (int x = entrance.clearApproach.min.x; x <= entrance.clearApproach.max.x; ++x)
					{
						const spk::Vector3Int world = box.destination + spk::rotateQuarterTurns(spk::Vector3Int{x, y, z} - p_instance.prefab->prefab.pivot(), turns);
						if (world.x == threshold.x && world.z == threshold.z)
						{
							continue;
						}
						TownColumn column{};
						try
						{
							column = p_workspace.townFromWorld({world.x, world.z});
						} catch (const std::out_of_range &)
						{
							p_rejection = {.category = TownRejectCategory::DoorClearance, .componentId = p_instance.id, .message = "door approach leaves town boundary"};
							return std::nullopt;
						}
						if (!validSurface(p_workspace, column, surface) || p_workspace.has(column, TownWorkspaceLayer::MainRoad) || p_workspace.has(column, TownWorkspaceLayer::BuildingSolid) || p_workspace.has(column, TownWorkspaceLayer::BuildingClearance))
						{
							p_rejection = {.category = TownRejectCategory::DoorClearance, .componentId = p_instance.id, .worldColumn = p_workspace.worldFromTown(column), .message = "door approach is blocked"};
							return std::nullopt;
						}
						approach.insert(column);
						longestApproach = std::max(longestApproach, std::abs(world.x - threshold.x) + std::abs(world.z - threshold.z));
					}
				}
			}
			if (approach.empty())
			{
				p_rejection = {.category = TownRejectCategory::DoorClearance, .componentId = p_instance.id, .message = "entrance contract has no clear approach columns"};
				return std::nullopt;
			}
			TownColumn connection{};
			try
			{
				connection = p_workspace.townFromWorld({threshold.x + outward.x * (longestApproach + 1), threshold.z + outward.z * (longestApproach + 1)});
			} catch (const std::out_of_range &)
			{
				p_rejection = {.category = TownRejectCategory::DoorClearance, .componentId = p_instance.id, .message = "connection point leaves town boundary"};
				return std::nullopt;
			}
			if (!validSurface(p_workspace, connection, surface) || p_workspace.has(connection, TownWorkspaceLayer::BuildingSolid) || p_workspace.has(connection, TownWorkspaceLayer::BuildingClearance) || p_workspace.has(connection, TownWorkspaceLayer::DoorApproach))
			{
				p_rejection = {.category = TownRejectCategory::DoorClearance, .componentId = p_instance.id, .worldColumn = p_workspace.worldFromTown(connection), .message = "connection point is not routeable"};
				return std::nullopt;
			}
			// Reserve one guaranteed configured-width lane from this connection point to the
			// existing main spine.  It is a placement-time feasibility reservation, not
			// an urban road: actual routes are still computed only after every building
			// has been accepted.  Later buildings cannot consume the only viable lane.
			std::vector<TownColumn> lane;
			bool reachedMainRoad = false;
			const int laneRadius = p_workspace.composition().layout.urbanRoadWidth / 2;
			for (int step = 0; step < p_workspace.width() && !reachedMainRoad; ++step)
			{
				const TownColumn center{connection.x + outward.x * step, connection.z + outward.z * step};
				for (int offset = -laneRadius; offset <= laneRadius; ++offset)
				{
					const TownColumn column{center.x - outward.z * offset, center.z + outward.x * offset};
					if (!validSurface(p_workspace, column, surface) || p_workspace.has(column, TownWorkspaceLayer::BuildingSolid) || p_workspace.has(column, TownWorkspaceLayer::BuildingClearance) || p_workspace.has(column, TownWorkspaceLayer::DoorThreshold) || p_workspace.has(column, TownWorkspaceLayer::DoorApproach))
					{
						p_rejection = {.category = TownRejectCategory::DoorClearance, .componentId = p_instance.id, .worldColumn = p_workspace.contains(column) ? std::optional<WorldColumn>{p_workspace.worldFromTown(column)} : std::nullopt, .message = "connection has no clear configured-width lane to the main road"};
						return std::nullopt;
					}
					lane.push_back(column);
				}
				reachedMainRoad = p_workspace.contains(center) && p_workspace.has(center, TownWorkspaceLayer::MainRoad);
			}
			if (!reachedMainRoad)
			{
				p_rejection = {.category = TownRejectCategory::DoorClearance, .componentId = p_instance.id, .message = "connection does not face a reachable main-road lane"};
				return std::nullopt;
			}
			for (TownColumn column : lane)
			{
				if (!p_workspace.has(column, TownWorkspaceLayer::MainRoad))
				{
					p_workspace.add(column, TownWorkspaceLayer::RoadsideReserved);
				}
			}
			for (TownColumn column : clearance)
			{
				p_workspace.add(column, TownWorkspaceLayer::BuildingClearance);
			}
			for (TownColumn column : solids)
			{
				if (!(column == thresholdColumn))
				{
					p_workspace.add(column, TownWorkspaceLayer::BuildingSolid);
				}
			}
			p_workspace.add(thresholdColumn, TownWorkspaceLayer::DoorThreshold);
			for (TownColumn column : approach)
			{
				p_workspace.add(column, TownWorkspaceLayer::DoorApproach);
			}
			p_claims.push_back(claim);
			return ResolvedTownEntrance{.buildingInstanceId = p_instance.id, .threshold = threshold, .outward = outward, .approachColumns = {approach.begin(), approach.end()}, .connectionPoint = connection};
		}

		[[nodiscard]] std::optional<RouteResult> findRoute(const TownWorkspace &p_workspace, TownColumn p_start, int p_turnPenalty)
		{
			const int width = p_workspace.width();
			const int stateCount = width * width * 5;
			const auto state = [width](TownColumn p_column, int p_direction) {
				return ((p_column.z * width + p_column.x) * 5) + p_direction;
			};
			const auto fromState = [width](int p_state) {
				const int cell = p_state / 5;
				return TownColumn{cell % width, cell / width};
			};
			const auto isGoal = [&](TownColumn p_column) {
				return p_workspace.has(p_column, TownWorkspaceLayer::MainRoad) || p_workspace.has(p_column, TownWorkspaceLayer::UrbanRoadSurface);
			};
			using Node = std::tuple<int, int, int>;
			std::priority_queue<Node, std::vector<Node>, std::greater<>> open;
			std::vector<int> best(static_cast<std::size_t>(stateCount), std::numeric_limits<int>::max());
			std::vector<int> previous(static_cast<std::size_t>(stateCount), -1);
			const int start = state(p_start, 4);
			const int routeLevel = p_workspace.surfaceHeight(p_start);
			best[start] = 0;
			open.push({0, 0, start});
			const std::array directions = {TownColumn{0, -1}, TownColumn{1, 0}, TownColumn{0, 1}, TownColumn{-1, 0}};
			while (!open.empty())
			{
				const auto [priority, cost, current] = open.top();
				(void)priority;
				open.pop();
				if (cost != best[current])
				{
					continue;
				}
				const TownColumn column = fromState(current);
				if (!(column == p_start) && isGoal(column))
				{
					std::vector<TownColumn> path;
					for (int cursor = current; cursor >= 0; cursor = previous[cursor])
					{
						path.push_back(fromState(cursor));
					}
					std::reverse(path.begin(), path.end());
					return RouteResult{.cost = cost, .path = std::move(path)};
				}
				const int incoming = current % 5;
				for (int direction = 0; direction < 4; ++direction)
				{
					const TownColumn next{column.x + directions[direction].x, column.z + directions[direction].z};
					if (!p_workspace.contains(next) || p_workspace.surfaceHeight(next) != routeLevel || p_workspace.has(next, TownWorkspaceLayer::TerrainBlocked) || p_workspace.has(next, TownWorkspaceLayer::Stair) || p_workspace.has(next, TownWorkspaceLayer::RouteBlocked) || p_workspace.has(next, TownWorkspaceLayer::BuildingSolid) || p_workspace.has(next, TownWorkspaceLayer::BuildingClearance) || p_workspace.has(next, TownWorkspaceLayer::DoorThreshold) || p_workspace.has(next, TownWorkspaceLayer::DoorApproach) || p_workspace.has(next, TownWorkspaceLayer::Dock))
					{
						continue;
					}
					const int nextState = state(next, direction);
					const bool road = p_workspace.has(next, TownWorkspaceLayer::MainRoad) || p_workspace.has(next, TownWorkspaceLayer::UrbanRoadSurface);
					const int nextCost = cost + (road ? 1 : 10) + (incoming != 4 && incoming != direction ? p_turnPenalty : 0);
					if (nextCost < best[nextState])
					{
						best[nextState] = nextCost;
						previous[nextState] = current;
						open.push({nextCost, nextCost, nextState});
					}
				}
			}
			return std::nullopt;
		}

		[[nodiscard]] std::vector<TownColumn> expandRoad(const std::vector<TownColumn> &p_path, int p_width, const ResolvedTownEntrance &p_entrance)
		{
			std::set<TownColumn> result;
			const int radius = p_width / 2;
			for (std::size_t index = 0; index < p_path.size(); ++index)
			{
				TownColumn direction{};
				if (index + 1 < p_path.size())
				{
					direction = {p_path[index + 1].x - p_path[index].x, p_path[index + 1].z - p_path[index].z};
				}
				else if (index > 0)
				{
					direction = {p_path[index].x - p_path[index - 1].x, p_path[index].z - p_path[index - 1].z};
				}
				const TownColumn lateral = p_path[index] == p_entrance.connectionPoint
											   ? TownColumn{-p_entrance.outward.z, p_entrance.outward.x}
											   : TownColumn{-direction.z, direction.x};
				for (int offset = -radius; offset <= radius; ++offset)
				{
					result.insert({p_path[index].x + lateral.x * offset, p_path[index].z + lateral.z * offset});
				}
			}
			return {result.begin(), result.end()};
		}

		[[nodiscard]] bool validateRoadSurface(const TownWorkspace &p_workspace, const std::vector<TownColumn> &p_surface, const ResolvedTownEntrance &p_entrance, std::string *p_reason = nullptr)
		{
			const int routeHeight = p_workspace.surfaceHeight(p_entrance.connectionPoint);
			for (TownColumn column : p_surface)
			{
				if (!validSurface(p_workspace, column, routeHeight) || p_workspace.has(column, TownWorkspaceLayer::BuildingSolid) || p_workspace.has(column, TownWorkspaceLayer::DoorThreshold) || p_workspace.has(column, TownWorkspaceLayer::DoorApproach) || p_workspace.has(column, TownWorkspaceLayer::BuildingClearance))
				{
					if (p_reason != nullptr)
					{
						*p_reason = "at " + std::to_string(column.x) + "," + std::to_string(column.z) + (p_workspace.contains(column) ? " layers=" + std::to_string(p_workspace.layers(column)) : " outside workspace");
					}
					return false;
				}
			}
			return true;
		}

		[[nodiscard]] std::optional<TownRejection> routeEntrances(TownWorkspace &p_workspace, TownCandidate &p_candidate, const TownComposition &p_composition, int &p_expansions)
		{
			std::set<std::string> remaining;
			for (const ResolvedTownEntrance &entrance : p_candidate.entrances)
			{
				remaining.insert(entrance.buildingInstanceId);
			}
			while (!remaining.empty())
			{
				const ResolvedTownEntrance *chosenEntrance = nullptr;
				std::optional<RouteResult> chosenRoute;
				for (const ResolvedTownEntrance &entrance : p_candidate.entrances)
				{
					if (!remaining.contains(entrance.buildingInstanceId))
					{
						continue;
					}
					const std::optional<RouteResult> route = findRoute(p_workspace, entrance.connectionPoint, p_composition.layout.routeTurnPenalty);
					if (!route)
					{
						continue;
					}
					if (!chosenRoute || std::tie(route->cost, entrance.buildingInstanceId, route->path) < std::tie(chosenRoute->cost, chosenEntrance->buildingInstanceId, chosenRoute->path))
					{
						chosenEntrance = &entrance;
						chosenRoute = route;
					}
				}
				if (!chosenRoute || chosenEntrance == nullptr)
				{
					return TownRejection{.category = TownRejectCategory::RouteUnavailable, .componentId = *remaining.begin(), .message = "no route from required connection point to the growing road network"};
				}
				std::vector<TownColumn> surface = expandRoad(chosenRoute->path, p_composition.layout.urbanRoadWidth, *chosenEntrance);
				std::string widthReason;
				bool widthValid = validateRoadSurface(p_workspace, surface, *chosenEntrance, &widthReason);
				for (int retry = 0; retry < 16 && !widthValid; ++retry)
				{
					for (TownColumn column : surface)
					{
						if (p_workspace.contains(column) && !p_workspace.has(column, TownWorkspaceLayer::MainRoad) && !p_workspace.has(column, TownWorkspaceLayer::UrbanRoadSurface))
						{
							p_workspace.add(column, TownWorkspaceLayer::RouteBlocked);
						}
					}
					chosenRoute = findRoute(p_workspace, chosenEntrance->connectionPoint, p_composition.layout.routeTurnPenalty);
					if (!chosenRoute)
					{
						break;
					}
					surface = expandRoad(chosenRoute->path, p_composition.layout.urbanRoadWidth, *chosenEntrance);
					widthValid = validateRoadSurface(p_workspace, surface, *chosenEntrance, &widthReason);
				}
				if (!chosenRoute || !widthValid)
				{
					return TownRejection{.category = TownRejectCategory::RoadWidthConflict, .componentId = chosenEntrance->buildingInstanceId, .message = "route centerline cannot support its full configured width " + widthReason};
				}
				for (TownColumn column : chosenRoute->path)
				{
					p_workspace.add(column, TownWorkspaceLayer::UrbanRoadCenterline);
				}
				for (TownColumn column : surface)
				{
					p_workspace.add(column, TownWorkspaceLayer::UrbanRoadSurface);
					const WorldColumn world = p_workspace.worldFromTown(column);
					p_candidate.urbanRoadSurface.push_back({.worldX = world.x, .worldZ = world.z, .surfaceY = p_workspace.surfaceHeight(column)});
				}
				UrbanRouteRecord record{.buildingInstanceId = chosenEntrance->buildingInstanceId, .cost = chosenRoute->cost};
				for (TownColumn column : chosenRoute->path)
				{
					const WorldColumn world = p_workspace.worldFromTown(column);
					record.centerline.emplace_back(world.x, world.z);
				}
				p_candidate.routes.push_back(std::move(record));
				remaining.erase(chosenEntrance->buildingInstanceId);
				p_expansions += static_cast<int>(chosenRoute->path.size());
			}
			std::sort(p_candidate.urbanRoadSurface.begin(), p_candidate.urbanRoadSurface.end());
			p_candidate.urbanRoadSurface.erase(std::unique(p_candidate.urbanRoadSurface.begin(), p_candidate.urbanRoadSurface.end()), p_candidate.urbanRoadSurface.end());
			return std::nullopt;
		}

		[[nodiscard]] bool sceneryCellEligible(const TownWorkspace &p_workspace, TownColumn p_column, bool p_roadside)
		{
			if (!p_workspace.has(p_column, TownWorkspaceLayer::Land) || p_workspace.has(p_column, TownWorkspaceLayer::TerrainBlocked))
			{
				return false;
			}
			const std::uint32_t forbidden = townLayer(TownWorkspaceLayer::MainRoad) | townLayer(TownWorkspaceLayer::UrbanRoadSurface) | townLayer(TownWorkspaceLayer::BuildingSolid) | townLayer(TownWorkspaceLayer::BuildingClearance) | townLayer(TownWorkspaceLayer::DoorThreshold) | townLayer(TownWorkspaceLayer::DoorApproach) | townLayer(TownWorkspaceLayer::Dock) | townLayer(TownWorkspaceLayer::Stair) | townLayer(TownWorkspaceLayer::RoadsideReserved) | townLayer(TownWorkspaceLayer::RoadScenery) | townLayer(TownWorkspaceLayer::GroundScenery);
			if ((p_workspace.layers(p_column) & forbidden) != 0)
			{
				return false;
			}
			if (!p_roadside)
			{
				return true;
			}
			for (const TownColumn delta : std::array{TownColumn{0, -1}, TownColumn{1, 0}, TownColumn{0, 1}, TownColumn{-1, 0}})
			{
				const TownColumn neighbor{p_column.x + delta.x, p_column.z + delta.z};
				if (p_workspace.contains(neighbor) && (p_workspace.has(neighbor, TownWorkspaceLayer::MainRoad) || p_workspace.has(neighbor, TownWorkspaceLayer::UrbanRoadSurface)))
				{
					return true;
				}
			}
			return false;
		}

		[[nodiscard]] std::optional<TownRejection> placeScenery(
			TownWorkspace &p_workspace,
			TownCandidate &p_candidate,
			const std::vector<TownSceneryRequest> &p_requests,
			bool p_roadside,
			const Registry<PrefabDefinition> &p_prefabs,
			std::uint64_t p_seed)
		{
			std::vector<std::pair<TownColumn, int>> placed;
			for (const TownSceneryRequest &request : p_requests)
			{
				const PrefabDefinition *prefab = p_prefabs.tryGet(request.prefabId);
				if (prefab == nullptr)
				{
					return TownRejection{.category = p_roadside ? TownRejectCategory::RoadSceneryUnavailable : TownRejectCategory::GroundSceneryUnavailable, .componentId = request.id, .message = "composition references an unknown scenery prefab"};
				}
				worldgen::Rng countRng(worldgen::deriveSeed(p_seed, "town/" + std::to_string(p_candidate.macroEntityIndex) + (p_roadside ? "/road-scenery/" : "/ground-scenery/") + request.id + "/count"));
				const int requested = request.count.minimum + (request.count.maximum > request.count.minimum ? countRng.below(request.count.maximum - request.count.minimum + 1) : 0);
				int placedCount = 0;
				worldgen::Rng orderRng(worldgen::deriveSeed(p_seed, "town/" + std::to_string(p_candidate.macroEntityIndex) + (p_roadside ? "/road-scenery/" : "/ground-scenery/") + request.id));
				std::vector<TownColumn> candidates;
				for (int z = 0; z < p_workspace.width(); ++z)
				{
					for (int x = 0; x < p_workspace.width(); ++x)
					{
						if (sceneryCellEligible(p_workspace, {x, z}, p_roadside))
						{
							candidates.push_back({x, z});
						}
					}
				}
				orderRng.shuffle(candidates);
				for (TownColumn column : candidates)
				{
					if (placedCount >= requested)
					{
						break;
					}
					const bool spaced = std::ranges::all_of(placed, [&](const auto &other) {
						const int dx = column.x - other.first.x, dz = column.z - other.first.z, spacing = std::max(request.spacing, other.second);
						return dx * dx + dz * dz >= spacing * spacing;
					});
					if (!spaced)
					{
						continue;
					}
					const WorldColumn world = p_workspace.worldFromTown(column);
					PrefabPlacement placement{.prefabId = request.prefabId, .anchor = {world.x, p_workspace.surfaceHeight(column) + 1, world.z}, .orientation = spk::orientationFromQuarterTurns(orderRng.below(4)), .foundation = false};
					const PlanClaim claim = placementClaim(*prefab, placement);
					const std::vector<TownColumn> footprint = columnsIn(p_workspace, claim);
					if (footprint.empty() || !std::ranges::all_of(footprint, [&](TownColumn cell) {
							return sceneryCellEligible(p_workspace, cell, p_roadside) && p_workspace.surfaceHeight(cell) == p_workspace.surfaceHeight(column);
						}))
					{
						continue;
					}
					if (std::ranges::any_of(p_candidate.buildingClaims, [&](const PlanClaim &other) {
							return overlaps(claim, other);
						}) ||
						std::ranges::any_of(p_candidate.sceneryClaims, [&](const PlanClaim &other) {
							return overlaps(claim, other);
						}))
					{
						continue;
					}
					for (TownColumn cell : footprint)
					{
						p_workspace.add(cell, p_roadside ? TownWorkspaceLayer::RoadScenery : TownWorkspaceLayer::GroundScenery);
					}
					p_candidate.sceneryClaims.push_back(claim);
					(p_roadside ? p_candidate.roadScenery : p_candidate.groundScenery).push_back(std::move(placement));
					placed.emplace_back(column, request.spacing);
					++placedCount;
				}
				if (request.required && placedCount < request.count.minimum)
				{
					return TownRejection{.category = p_roadside ? TownRejectCategory::RoadSceneryUnavailable : TownRejectCategory::GroundSceneryUnavailable, .componentId = request.id, .message = "required scenery minimum cannot fit after roads and approaches"};
				}
			}
			return std::nullopt;
		}

		[[nodiscard]] std::optional<TownRejection> planPortDock(const TownWorkspace &p_workspace, TownCandidate &p_candidate)
		{
			const TownWorldBounds bounds = p_workspace.bounds();
			for (int z = 0; z < p_workspace.width(); ++z)
			{
				for (int x = 0; x < p_workspace.width(); ++x)
				{
					const TownColumn cell{x, z};
					if (!p_workspace.has(cell, TownWorkspaceLayer::Water))
					{
						continue;
					}
					bool edge = x == 0 || z == 0 || x + 1 == p_workspace.width() || z + 1 == p_workspace.width();
					if (!edge)
					{
						continue;
					}
					const WorldColumn world = p_workspace.worldFromTown(cell);
					const PlanCell macro = p_workspace.planCellFromWorld(world);
					p_candidate.dockCells.emplace_back(macro.row, macro.col);
					p_candidate.boardingEndpoint = spk::Vector3Int{world.x, 1, world.z};
					p_candidate.dockClaims.push_back({.min = {world.x, 0, world.z}, .max = {world.x, 2, world.z}});
					return std::nullopt;
				}
			}
			return TownRejection{.category = TownRejectCategory::WaterfrontMismatch, .componentId = "dock", .message = "port reservation has no reachable waterfront column"};
		}

		void collectBoundaryCells(const TownWorkspace &p_workspace, TownCandidate &p_candidate)
		{
			std::set<std::pair<int, int>> boundary;
			for (int z = 0; z < p_workspace.width(); ++z)
			{
				for (int x = 0; x < p_workspace.width(); ++x)
				{
					const PlanCell cell = p_workspace.planCellFromWorld(p_workspace.worldFromTown({x, z}));
					if (cell.row >= 0 && cell.col >= 0 && cell.row < 4096 && cell.col < 4096)
					{
						boundary.emplace(cell.row, cell.col);
					}
				}
			}
			p_candidate.boundaryCells.assign(boundary.begin(), boundary.end());
		}
	}

	TownPlanningError::TownPlanningError(std::size_t p_macroEntityIndex, std::string p_compositionId, TownRejection p_rejection, std::map<std::string, int> p_counts, std::string p_message) :
		std::runtime_error(std::move(p_message)),
		macroEntityIndex(p_macroEntityIndex),
		compositionId(std::move(p_compositionId)),
		rejection(std::move(p_rejection)),
		rejectionCounts(std::move(p_counts))
	{
	}

	TownMutationSnapshot snapshotTownMutation(const WorldPlan &p_plan) noexcept
	{
		return {.placements = p_plan.placements.size(), .stairways = p_plan.stairways.size(), .portals = p_plan.portals.size(), .towns = p_plan.towns.size(), .claims = p_plan.townClaims.size(), .boatLinks = p_plan.boatLinks.size(), .warnings = p_plan.stats.warnings.size()};
	}

	bool matchesTownMutationSnapshot(const WorldPlan &p_plan, TownMutationSnapshot p_snapshot) noexcept
	{
		return snapshotTownMutation(p_plan) == p_snapshot;
	}

	std::optional<int> deriveTownRadius(
		const TownComposition &p_composition,
		const Registry<PrefabDefinition> &p_prefabs,
		const PlanTown &p_biomeTown,
		std::uint64_t p_worldSeed,
		std::size_t p_macroEntityIndex,
		TownRejection &p_rejection)
	{
		const std::optional<std::vector<BuildingInstance>> expanded =
			expandInstances(p_composition, p_biomeTown, p_prefabs, p_worldSeed, p_macroEntityIndex, p_rejection);
		if (!expanded)
		{
			return std::nullopt;
		}

		struct BuildingFootprint
		{
			int alongStreet = 0;
			int awayFromStreet = 0;
			int approach = 0;
			int spacing = 0;
		};
		std::vector<BuildingFootprint> footprints;
		for (const BuildingInstance &instance : *expanded)
		{
			const spk::Vector3Int localMin = instance.prefab->clearance ? instance.prefab->clearance->min : instance.prefab->prefab.minBounds();
			const spk::Vector3Int localMax = instance.prefab->clearance ? instance.prefab->clearance->max : instance.prefab->prefab.maxBounds();
			const PrefabEntrance &entrance = *instance.prefab->entrance;
			const PrefabAnchor *door = instance.prefab->tryAnchor(entrance.anchorName);
			int approach = 0;
			switch (entrance.outwardFacing)
			{
			case spk::VoxelOrientation::PositiveX:
				approach = entrance.clearApproach.max.x - door->position.x;
				break;
			case spk::VoxelOrientation::NegativeX:
				approach = door->position.x - entrance.clearApproach.min.x;
				break;
			case spk::VoxelOrientation::PositiveZ:
				approach = entrance.clearApproach.max.z - door->position.z;
				break;
			case spk::VoxelOrientation::NegativeZ:
				approach = door->position.z - entrance.clearApproach.min.z;
				break;
			}
			footprints.push_back({.alongStreet = localMax.x - localMin.x + 1, .awayFromStreet = localMax.z - localMin.z + 1, .approach = std::max(0, approach), .spacing = std::max(instance.spacing, p_composition.layout.minimumBuildingSpacing)});
		}
		std::sort(footprints.begin(), footprints.end(), [](const BuildingFootprint &p_left, const BuildingFootprint &p_right) {
			return std::tie(p_left.alongStreet, p_left.awayFromStreet) > std::tie(p_right.alongStreet, p_right.awayFromStreet);
		});
		std::array<int, 2> streetLengths{};
		int largestDepth = 0;
		int largestApproach = 0;
		for (const BuildingFootprint &footprint : footprints)
		{
			const int side = streetLengths[0] <= streetLengths[1] ? 0 : 1;
			if (streetLengths[side] != 0)
			{
				streetLengths[side] += footprint.spacing;
			}
			streetLengths[side] += footprint.alongStreet;
			largestDepth = std::max(largestDepth, footprint.awayFromStreet);
			largestApproach = std::max(largestApproach, footprint.approach);
		}

		int sceneryArea = 0;
		const auto addSceneryArea = [&](const std::vector<TownSceneryRequest> &p_requests, const std::string &p_channel) {
			for (const TownSceneryRequest &request : p_requests)
			{
				const PrefabDefinition *prefab = p_prefabs.tryGet(request.prefabId);
				if (prefab == nullptr)
				{
					p_rejection = {.category = TownRejectCategory::MissingContent, .componentId = request.id, .message = "composition references an unknown scenery prefab"};
					return false;
				}
				worldgen::Rng rng(worldgen::deriveSeed(p_worldSeed, "town/" + std::to_string(p_macroEntityIndex) + "/" + p_channel + "/" + request.id + "/count"));
				const int count = request.count.minimum + (request.count.maximum > request.count.minimum ? rng.below(request.count.maximum - request.count.minimum + 1) : 0);
				const spk::Vector3Int localMin = prefab->clearance ? prefab->clearance->min : prefab->prefab.minBounds();
				const spk::Vector3Int localMax = prefab->clearance ? prefab->clearance->max : prefab->prefab.maxBounds();
				sceneryArea += count * (localMax.x - localMin.x + 1) * (localMax.z - localMin.z + 1);
			}
			return true;
		};
		if (!addSceneryArea(p_composition.roadScenery, "road-scenery") || !addSceneryArea(p_composition.groundScenery, "ground-scenery"))
		{
			return std::nullopt;
		}

		const int roadHalfWidth = (p_composition.layout.mainRoadWidth + 1) / 2;
		const int laneHalfWidth = (p_composition.layout.urbanRoadWidth + 1) / 2;
		const int streetHalfLength = (std::max(streetLengths[0], streetLengths[1]) + 1) / 2;
		const int streetHalfDepth = roadHalfWidth + laneHalfWidth + largestApproach + largestDepth;
		const int sceneryMargin = std::max(2, static_cast<int>(std::ceil(std::sqrt(static_cast<double>(sceneryArea)))) / 2);
		// The planner needs room to choose among several road-facing addresses on
		// uneven terrain, not just the mathematical rectangle of the footprints.
		// Scale that search apron from the largest building so a larger composition
		// naturally obtains a larger, still compact envelope.
		const int structuralMargin = std::max(6, (largestDepth + 1) / 2) + 1;
		return std::max(12, std::max(streetHalfLength, streetHalfDepth) + p_composition.layout.minimumBuildingSpacing + sceneryMargin + structuralMargin);
	}

	TownPlanResult planTown(const WorldPlan &p_plan, const PlanTownSite &p_site, const TownComposition &p_composition, const Registry<PrefabDefinition> &p_prefabs, const PlanTown &p_biomeTown, std::uint64_t p_worldSeed)
	{
		TownPlanResult result;
		if (!isSettlement(p_site.kind) || p_composition.kind != compositionKindFor(p_site.kind))
		{
			result.rejection = TownRejection{.category = TownRejectCategory::SiteTerrain, .componentId = p_site.compositionId, .message = "site kind and composition kind differ"};
			return result;
		}
		TownRejection contentFailure;
		const std::optional<std::vector<BuildingInstance>> expanded = expandInstances(p_composition, p_biomeTown, p_prefabs, p_worldSeed, p_site.macroEntityIndex, contentFailure);
		if (!expanded)
		{
			result.rejection = contentFailure;
			return result;
		}
		for (int attempt = 0; attempt < p_composition.layout.layoutAttempts; ++attempt)
		{
			++result.layoutAttempts;
			TownWorkspace workspace(p_plan, p_site, p_composition);
			TownCandidate candidate{.compositionId = p_composition.id, .macroEntityIndex = p_site.macroEntityIndex, .kind = p_site.kind, .bounds = workspace.bounds(), .centerRow = p_site.centerRow, .centerCol = p_site.centerCol, .mainRoadSurface = workspace.mainRoadSurface()};
			collectBoundaryCells(workspace, candidate);
			if (candidate.mainRoadSurface.empty())
			{
				result.rejection = TownRejection{.category = TownRejectCategory::SiteTerrain, .componentId = p_composition.id, .layoutAttempt = attempt, .message = "global town spine did not project into workspace"};
				continue;
			}
			std::vector<BuildingInstance> instances = *expanded;
			sortInstances(instances, worldgen::deriveSeed(p_worldSeed, "town/" + std::to_string(p_site.macroEntityIndex) + "/layout/" + std::to_string(attempt)), p_site.macroEntityIndex);
			TownRejection failure;
			bool requiredFailed = false;
			for (const BuildingInstance &instance : instances)
			{
				bool placed = false;
				const PrefabAnchor *door = instance.prefab->tryAnchor(instance.prefab->entrance->anchorName);
				struct BuildingCandidate
				{
					TownColumn threshold;
					int turns = 0;
				};
				std::vector<BuildingCandidate> buildingCandidates;
				std::set<std::tuple<int, int, int>> seenCandidates;
				const int workspaceCenter = workspace.width() / 2;
				const int compactRoadReach = std::min(20, std::max(8, p_site.radiusColumns - 12));
				const int maximumDoorDistance = std::min(16, std::max(6, p_site.radiusColumns - 10));
				const auto addCandidatesBesideRoad = [&](bool p_compactOnly) {
					for (TownColumn road : workspace.columnsWith(TownWorkspaceLayer::MainRoad))
					{
						if (p_compactOnly && std::abs(road.x - workspaceCenter) + std::abs(road.z - workspaceCenter) > compactRoadReach)
						{
							continue;
						}
						for (const TownColumn outward : std::array{TownColumn{0, -1}, TownColumn{1, 0}, TownColumn{0, 1}, TownColumn{-1, 0}})
						{
							for (int distance = 4; distance <= maximumDoorDistance; ++distance)
							{
								const TownColumn threshold{road.x - outward.x * distance, road.z - outward.z * distance};
								if (!workspace.contains(threshold) || workspace.surfaceHeight(threshold) != workspace.surfaceHeight(road))
								{
									continue;
								}
								for (int turns = 0; turns < 4; ++turns)
								{
									const spk::Vector3Int facade = outwardFor(turns, instance.prefab->entrance->outwardFacing);
									if (facade.x != outward.x || facade.z != outward.z)
									{
										continue;
									}
									if (seenCandidates.insert({threshold.x, threshold.z, turns}).second)
									{
										buildingCandidates.push_back({.threshold = threshold, .turns = turns});
									}
								}
							}
						}
					}
				};
				// Buildings are addressed from the spine outward.  This makes the
				// settlement read as a compact street block rather than a handful of
				// isolated structures scattered across the reservation.  A broader
				// in-bounds road pass is retained solely as a deterministic fallback
				// for a short or off-centre incoming road.
				addCandidatesBesideRoad(true);
				if (buildingCandidates.empty())
				{
					addCandidatesBesideRoad(false);
				}
				worldgen::Rng candidateRng(worldgen::deriveSeed(p_worldSeed, "town/" + std::to_string(p_site.macroEntityIndex) + "/layout/" + std::to_string(attempt) + "/buildings/" + instance.id + "/candidates"));
				candidateRng.shuffle(buildingCandidates);
				for (int candidateIndex = 0; candidateIndex < std::min<int>(p_composition.layout.buildingAttemptsPerItem, static_cast<int>(buildingCandidates.size())); ++candidateIndex)
				{
					++result.buildingCandidates;
					const BuildingCandidate &candidateAddress = buildingCandidates[candidateIndex];
					const WorldColumn thresholdWorld = workspace.worldFromTown(candidateAddress.threshold);
					const int x = thresholdWorld.x;
					const int z = thresholdWorld.z;
					const int turns = candidateAddress.turns;
					const spk::Vector3Int delta = spk::rotateQuarterTurns(door->position - instance.prefab->prefab.pivot(), turns);
					PrefabPlacement placement{.prefabId = instance.prefabId, .anchor = {x - delta.x, workspace.surfaceHeight(candidateAddress.threshold) - delta.y, z - delta.z}, .orientation = spk::orientationFromQuarterTurns(turns), .foundation = true, .anchorToPivot = true, .townRole = instance.role};
					if (const std::optional<ResolvedTownEntrance> entrance = validateAndPlaceBuilding(workspace, p_plan, instance, placement, candidate.buildingClaims, failure))
					{
						candidate.buildings.push_back(std::move(placement));
						candidate.entrances.push_back(*entrance);
						placed = true;
						break;
					}
				}
				if (!placed && instance.required)
				{
					requiredFailed = true;
					failure.layoutAttempt = attempt;
					break;
				}
			}
			if (requiredFailed)
			{
				if (failure.message.empty())
				{
					failure = {.category = TownRejectCategory::BuildingPlacement, .componentId = "building", .layoutAttempt = attempt, .message = "bounded building placement attempts exhausted"};
				}
				result.rejection = failure;
				continue;
			}
			if (const std::optional<TownRejection> routeFailure = routeEntrances(workspace, candidate, p_composition, result.routeExpansions))
			{
				result.rejection = *routeFailure;
				result.rejection->layoutAttempt = attempt;
				continue;
			}
			if (p_site.kind == PlanEntityKind::PortCity)
			{
				if (const std::optional<TownRejection> dockFailure = planPortDock(workspace, candidate))
				{
					result.rejection = *dockFailure;
					result.rejection->layoutAttempt = attempt;
					continue;
				}
			}
			if (const std::optional<TownRejection> sceneryFailure = placeScenery(workspace, candidate, p_composition.roadScenery, true, p_prefabs, p_worldSeed))
			{
				result.rejection = *sceneryFailure;
				result.rejection->layoutAttempt = attempt;
				continue;
			}
			if (const std::optional<TownRejection> sceneryFailure = placeScenery(workspace, candidate, p_composition.groundScenery, false, p_prefabs, p_worldSeed))
			{
				result.rejection = *sceneryFailure;
				result.rejection->layoutAttempt = attempt;
				continue;
			}
			result.candidate = std::move(candidate);
			return result;
		}
		return result;
	}

	std::string renderTownWorkspaceAscii(const TownWorkspace &p_workspace)
	{
		std::string result;
		for (int z = 0; z < p_workspace.width(); ++z)
		{
			for (int x = 0; x < p_workspace.width(); ++x)
			{
				const TownColumn column{x, z};
				char glyph = p_workspace.has(column, TownWorkspaceLayer::Water) ? '~' : '.';
				if (p_workspace.has(column, TownWorkspaceLayer::MainRoad))
				{
					glyph = 'M';
				}
				if (p_workspace.has(column, TownWorkspaceLayer::UrbanRoadSurface))
				{
					glyph = 'R';
				}
				if (p_workspace.has(column, TownWorkspaceLayer::BuildingSolid))
				{
					glyph = '#';
				}
				if (p_workspace.has(column, TownWorkspaceLayer::DoorThreshold))
				{
					glyph = 'D';
				}
				if (p_workspace.has(column, TownWorkspaceLayer::DoorApproach))
				{
					glyph = 'a';
				}
				if (p_workspace.has(column, TownWorkspaceLayer::RoadScenery))
				{
					glyph = 'r';
				}
				if (p_workspace.has(column, TownWorkspaceLayer::GroundScenery))
				{
					glyph = 'g';
				}
				result += glyph;
			}
			result += '\n';
		}
		return result;
	}
}
