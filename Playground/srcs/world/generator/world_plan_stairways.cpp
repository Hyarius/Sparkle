#include "world/generator/world_plan_generator.hpp"

#include "structures/voxel/spk_voxel_orientation.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <iterator>
#include <optional>
#include <set>
#include <string>
#include <utility>
#include <vector>

// Claimed zones and staircases. Every cliff crossing is shaped into a
// StairCandidate by one of three layout builders (one pass along the cliff,
// switchback, perpendicular) and funneled through the single
// tryCommitStairCandidate gate, which validates footprints, claims the zone, and
// records the committed staircase as a PlanStairway on the plan.
namespace pg::worldgen
{
	// ---------------- Claimed zones ----------------
	// Resolution lives in prefab_placement_math.hpp, shared with
	// PlanChunkProvider, so the plan reasons about the exact voxels the
	// realization will stamp.
	std::optional<ResolvedPlacementBox> Generator::resolveBox(const PrefabPlacement &p_placement) const
	{
		const PrefabDefinition *definition = prefabs.tryGet(p_placement.prefabId);
		if (definition == nullptr)
		{
			return std::nullopt;
		}
		return resolvePlacement(definition->prefab, p_placement);
	}

	std::optional<Generator::Claim> Generator::claimBoxFor(const PrefabPlacement &p_placement) const
	{
		const PrefabDefinition *definition = prefabs.tryGet(p_placement.prefabId);
		const std::optional<ResolvedPlacementBox> resolved = resolveBox(p_placement);
		if (definition == nullptr || !resolved.has_value())
		{
			return std::nullopt;
		}
		const spk::Vector3Int pivot = definition->prefab.pivot();
		const spk::Vector3Int localMin =
			definition->clearance.has_value() ? definition->clearance->min : definition->prefab.minBounds();
		const spk::Vector3Int localMax =
			definition->clearance.has_value() ? definition->clearance->max : definition->prefab.maxBounds();
		// Same quarter-turn convention as spk::Prefab (rotations around +Y through
		// the pivot), so claimed zones rotate exactly like the content.
		const int turns = spk::quarterTurnsOf(p_placement.orientation);
		const spk::Vector3Int a = spk::rotateQuarterTurns(localMin - pivot, turns);
		const spk::Vector3Int b = spk::rotateQuarterTurns(localMax - pivot, turns);
		return Claim{
			.min = resolved->destination + spk::Vector3Int{std::min(a.x, b.x), std::min(a.y, b.y), std::min(a.z, b.z)},
			.max = resolved->destination + spk::Vector3Int{std::max(a.x, b.x), std::max(a.y, b.y), std::max(a.z, b.z)}};
	}

	bool Generator::claimsOverlap(const Claim &p_first, const Claim &p_second)
	{
		return p_first.min.x <= p_second.max.x && p_first.max.x >= p_second.min.x &&
			   p_first.min.y <= p_second.max.y && p_first.max.y >= p_second.min.y &&
			   p_first.min.z <= p_second.max.z && p_first.max.z >= p_second.min.z;
	}

	bool Generator::zoneIsFree(const Claim &p_claim) const
	{
		return std::ranges::none_of(hardClaims, [&](const Claim &p_placed) {
			return claimsOverlap(p_claim, p_placed);
		});
	}

	// ---------------- Stair prefab pools ----------------
	// Stair prefabs are synthesized from each biome's palette voxels (see
	// synthesizeClimbPrefabs) and carried on PlanBiome as id pools: road climbs draw
	// from the biome's stair pool, wild climbs from its slope pool. Flight pools hold
	// several pre-mixed variants; pickStairLength draws one per segment for variety. A
	// biome without stair/slope voxels, or a cell outside any zone, falls back to the
	// shared "stair-length"/"stair-platform" pair.
	std::vector<std::string> Generator::stairLengthPoolFor(int p_zone, bool p_onRoad) const
	{
		if (p_zone >= 0)
		{
			const PlanBiome &biome = plan.biomes[plan.zones[p_zone].biomeIndex];
			const std::vector<std::string> &pool = p_onRoad ? biome.roadStairLengths : biome.wildSlopeLengths;
			if (!pool.empty())
			{
				return pool;
			}
		}
		return {"stair-length"};
	}

	std::string Generator::pickStairLength(int p_zone, bool p_onRoad)
	{
		return pickPrefab(stairLengthPoolFor(p_zone, p_onRoad));
	}

	std::string Generator::stairPlatformPrefabFor(int p_zone, bool p_onRoad) const
	{
		if (p_zone >= 0)
		{
			const PlanBiome &biome = plan.biomes[plan.zones[p_zone].biomeIndex];
			const std::string &id = p_onRoad ? biome.roadStairPlatform : biome.wildSlopePlatform;
			if (!id.empty())
			{
				return id;
			}
		}
		return "stair-platform";
	}

	// ---------------- Footprint validation ----------------
	std::optional<Generator::StairFootprint> Generator::stairFootprintOf(const PrefabPlacement &p_placement) const
	{
		const std::optional<ResolvedPlacementBox> resolved = resolveBox(p_placement);
		if (!resolved.has_value())
		{
			return std::nullopt;
		}
		return StairFootprint{
			.minX = resolved->worldMin.x,
			.maxX = resolved->worldMin.x + resolved->extents.x - 1,
			.minZ = resolved->worldMin.z,
			.maxZ = resolved->worldMin.z + resolved->extents.z - 1};
	}

	bool Generator::footprintsOverlap(
		const StairFootprint &p_first,
		const StairFootprint &p_second)
	{
		return p_first.minX <= p_second.maxX && p_first.maxX >= p_second.minX &&
			   p_first.minZ <= p_second.maxZ && p_first.maxZ >= p_second.minZ;
	}

	bool Generator::planCellHasEntity(int p_row, int p_col) const
	{
		return std::ranges::any_of(plan.entities, [&](const PlanEntity &p_entity) {
			return p_entity.row == p_row && p_entity.col == p_col;
		});
	}

	bool Generator::stairFootprintFits(
		const StairFootprint &p_footprint,
		int p_lowLevel,
		int p_lowZone,
		RoadRule p_roadRule,
		const std::vector<StairFootprint> &p_group) const
	{
		for (const StairFootprint &placed : stairFootprints)
		{
			if (footprintsOverlap(p_footprint, placed))
			{
				return false;
			}
		}
		for (const StairFootprint &placed : p_group)
		{
			if (footprintsOverlap(p_footprint, placed))
			{
				return false;
			}
		}

		for (int worldZ = p_footprint.minZ; worldZ <= p_footprint.maxZ; ++worldZ)
		{
			for (int worldX = p_footprint.minX; worldX <= p_footprint.maxX; ++worldX)
			{
				const int row = plan.cellIndexFromWorld(worldZ);
				const int col = plan.cellIndexFromWorld(worldX);
				if (!plan.land.contains(row, col) || !isLand(row, col) ||
					plan.height.at(row, col) != p_lowLevel || plan.water.at(row, col) != 0 ||
					plan.bridge.at(row, col) != 0 || planCellHasEntity(row, col))
				{
					return false;
				}
				switch (p_roadRule)
				{
				case RoadRule::Require:
					if (plan.road.at(row, col) == 0)
					{
						return false;
					}
					break;
				case RoadRule::Allow:
					if (plan.zone.at(row, col) != p_lowZone)
					{
						return false;
					}
					break;
				case RoadRule::Forbid:
					if (plan.road.at(row, col) != 0 || plan.zone.at(row, col) != p_lowZone)
					{
						return false;
					}
					break;
				}
			}
		}
		return true;
	}

	// ---------------- Candidate commit ----------------
	// The one commit gate every layout goes through: validates the candidate's
	// stamped and checked rectangles, claims its zones, appends its placements
	// to the plan as one contiguous run, and records the staircase.
	bool Generator::tryCommitStairCandidate(const StairSite &p_site, StairCandidate &&p_candidate)
	{
		// Straight ramps crossing on the road network must stay on road cells,
		// composed road climbs may also use clear terrain beside the road, and
		// wild stairways must not touch roads at all.
		const RoadRule roadRule = p_site.onRoad
									  ? (p_site.steps == 1 ? RoadRule::Require : RoadRule::Allow)
									  : RoadRule::Forbid;
		std::vector<StairFootprint> footprints;
		footprints.reserve(p_candidate.placements.size() + 2);
		for (const PrefabPlacement &placement : p_candidate.placements)
		{
			const std::optional<StairFootprint> footprint = stairFootprintOf(placement);
			if (!footprint.has_value() ||
				!stairFootprintFits(*footprint, p_site.lowLevel, p_site.lowZone, roadRule, footprints))
			{
				return false;
			}
			footprints.push_back(*footprint);
		}
		// The check-only band (the approach lane beside a composed road climb):
		// validated and reserved exactly like stamped footprints, never realized.
		if (p_candidate.checkedBand.has_value())
		{
			if (!stairFootprintFits(*p_candidate.checkedBand, p_site.lowLevel, p_site.lowZone, roadRule, footprints))
			{
				return false;
			}
			footprints.push_back(*p_candidate.checkedBand);
		}
		// The reserved exit (the plateau cells beyond the top platform): only
		// tested against other stairways, then recorded, so two flights can never
		// meet face to face across a boundary. Its terrain was validated by the
		// layout builder.
		if (p_candidate.reservedExit.has_value())
		{
			const StairFootprint &reserved = *p_candidate.reservedExit;
			const bool blocked =
				std::ranges::any_of(
					stairFootprints,
					[&](const StairFootprint &p_placed) { return footprintsOverlap(reserved, p_placed); }) ||
				std::ranges::any_of(footprints, [&](const StairFootprint &p_placed) {
					return footprintsOverlap(reserved, p_placed);
				});
			if (blocked)
			{
				return false;
			}
			footprints.push_back(reserved);
		}
		// Stairways are placed first and have priority: they claim their zones
		// unconditionally, and everything placed later must keep clear of them.
		for (const PrefabPlacement &placement : p_candidate.placements)
		{
			if (const std::optional<Claim> claim = claimBoxFor(placement); claim.has_value())
			{
				hardClaims.push_back(*claim);
			}
		}
		hardClaims.insert(hardClaims.end(), p_candidate.claims.begin(), p_candidate.claims.end());

		PlanStairway &record = p_candidate.record;
		record.firstPlacement = plan.placements.size();
		record.placementCount = p_candidate.placements.size();
		record.lowRow = p_site.lowRow;
		record.lowCol = p_site.lowCol;
		record.steps = p_site.steps;
		record.road = p_site.onRoad;
		// The preview map repaints these rectangles in road color: where a climb
		// detours the walked path, the drawn road follows the actual structure.
		record.footprints.reserve(footprints.size());
		for (const StairFootprint &footprint : footprints)
		{
			record.footprints.push_back(
				{.minX = footprint.minX, .maxX = footprint.maxX, .minZ = footprint.minZ, .maxZ = footprint.maxZ});
		}
		plan.placements.insert(
			plan.placements.end(),
			std::make_move_iterator(p_candidate.placements.begin()),
			std::make_move_iterator(p_candidate.placements.end()));
		stairFootprints.insert(stairFootprints.end(), footprints.begin(), footprints.end());
		for (const StairFootprint &footprint : footprints)
		{
			const int minRow = plan.cellIndexFromWorld(footprint.minZ);
			const int maxRow = plan.cellIndexFromWorld(footprint.maxZ);
			const int minCol = plan.cellIndexFromWorld(footprint.minX);
			const int maxCol = plan.cellIndexFromWorld(footprint.maxX);
			for (int row = minRow; row <= maxRow; ++row)
			{
				for (int col = minCol; col <= maxCol; ++col)
				{
					if (std::ranges::find(stairCells, Cell{row, col}) == stairCells.end())
					{
						stairCells.push_back({row, col});
					}
				}
			}
		}
		stairCells.push_back({p_site.lowRow, p_site.lowCol});
		if (record.steps >= 2)
		{
			++plan.stats.composedStairPlacements;
		}
		plan.stairways.push_back(std::move(record));
		return true;
	}

	// Shared tail of both composed layouts: the three high-plateau cells the top
	// platform opens onto must be dry, level land; they are reserved so no other
	// flight or building ever blocks the exit face to face. Road climbs also get
	// their approach band checked and paved, plus one claim over the whole
	// structure (band included, low ground to above the top platform) that keeps
	// scenery and buildings out from under the flight and off the approach.
	bool Generator::guardComposedCandidate(
		const StairSite &p_site,
		StairCandidate &p_candidate,
		const StairFootprint &p_band,
		int p_alongMin,
		int p_alongMax,
		int p_topAlong)
	{
		const CliffFrame &frame = p_site.frame;
		const int exitColumn = p_site.wallColumn - p_site.wallSide;
		for (int along = p_topAlong - 1; along <= p_topAlong + 1; ++along)
		{
			const Cell exitCell = frame.cell(exitColumn, along);
			const bool clear = plan.land.contains(exitCell.row, exitCell.col) &&
							   isLand(exitCell.row, exitCell.col) &&
							   plan.height.at(exitCell.row, exitCell.col) == p_site.highLevel &&
							   plan.water.at(exitCell.row, exitCell.col) == 0 &&
							   plan.bridge.at(exitCell.row, exitCell.col) == 0;
			if (!clear)
			{
				return false;
			}
		}
		p_candidate.reservedExit = frame.rect(exitColumn, exitColumn, p_topAlong - 1, p_topAlong + 1);
		if (p_site.onRoad)
		{
			const int bandAcrossMin = frame.acrossIsX ? p_band.minX : p_band.minZ;
			const int bandAcrossMax = frame.acrossIsX ? p_band.maxX : p_band.maxZ;
			p_candidate.checkedBand = p_band;
			p_candidate.claims.push_back(frame.claim(
				std::min(p_site.wallColumn, bandAcrossMin),
				std::max(p_site.wallColumn, bandAcrossMax),
				p_site.surface,
				p_site.highSurface + 4,
				p_alongMin,
				p_alongMax));
			p_candidate.record.pavedApproach =
				PlanStairRect{.minX = p_band.minX, .maxX = p_band.maxX, .minZ = p_band.minZ, .maxZ = p_band.maxZ};
		}
		p_candidate.claims.push_back(frame.claim(
			exitColumn, exitColumn, p_site.highSurface + 1, p_site.highSurface + 4, p_topAlong - 1, p_topAlong + 1));
		p_candidate.record.plateauCell = frame.at(exitColumn, 0, p_topAlong);
		return true;
	}

	// One run along the cliff. The three across-columns nearest the wall carry
	// the platforms and the flight: the top platform pads the 3-wide road strip
	// of the crossing so stepping over the boundary lands on it flush, then the
	// flight descends beside the cliff toward the bottom platform. The fourth
	// column is a checked, unstamped walkway lane connecting the road dead-end
	// below the top platform to the bottom platform.
	std::optional<Generator::StairCandidate> Generator::makeOnePassCandidate(
		const StairSite &p_site,
		const std::string &p_platformId,
		int p_tangent,
		int p_crossOffset)
	{
		const CliffFrame &frame = p_site.frame;
		const int run = cfg.blocksPerLevel; // ramp run == ramp rise (cube prefab)
		const int steps = p_site.steps;
		const int acrossCenter = p_site.wallColumn + p_site.wallSide;
		// The three columns beyond the structure form the paved approach band:
		// the road-width path from the crossing dead-end to the bottom platform.
		const int bandNearColumn = p_site.wallColumn + 3 * p_site.wallSide;
		const int bandFarColumn = p_site.wallColumn + 5 * p_site.wallSide;
		const int alongCenter = p_site.alongCenterBase + p_crossOffset;

		StairCandidate candidate;
		candidate.record.layout = StairLayout::OnePass;
		candidate.record.topAnchor = frame.at(acrossCenter, p_site.highSurface + 1, alongCenter);
		candidate.record.bottomAnchor =
			frame.at(acrossCenter, p_site.surface + 1, alongCenter + p_tangent * run * (steps + 1));
		candidate.placements.reserve(static_cast<std::size_t>(steps) + 2);

		PrefabPlacement top;
		top.prefabId = p_platformId;
		top.foundation = true;
		top.anchor = candidate.record.topAnchor;
		candidate.placements.push_back(std::move(top));

		for (int segment = 1; segment <= steps; ++segment)
		{
			PrefabPlacement piece;
			piece.prefabId = pickStairLength(p_site.lowZone, p_site.onRoad);
			piece.orientation = frame.alongAscend(p_tangent);
			piece.foundation = true;
			piece.anchor = frame.at(
				acrossCenter,
				p_site.surface + 1 + run * (steps - segment),
				alongCenter + p_tangent * run * segment);
			candidate.placements.push_back(std::move(piece));
		}

		PrefabPlacement bottom;
		bottom.prefabId = p_platformId;
		bottom.foundation = true;
		bottom.anchor = candidate.record.bottomAnchor;
		candidate.placements.push_back(std::move(bottom));

		const int nearEdge = alongCenter - p_tangent;
		const int farEdge = alongCenter + p_tangent * (run * (steps + 1) + 1);
		const int alongMin = std::min(nearEdge, farEdge);
		const int alongMax = std::max(nearEdge, farEdge);
		const StairFootprint band = frame.rect(
			std::min(bandNearColumn, bandFarColumn), std::max(bandNearColumn, bandFarColumn), alongMin, alongMax);
		candidate.record.centerPath.reserve(static_cast<std::size_t>(run * (steps + 1) + 1));
		for (int distance = 0; distance <= run * (steps + 1); ++distance)
		{
			candidate.record.centerPath.push_back(frame.at(acrossCenter, 0, alongCenter + p_tangent * distance));
		}
		if (!guardComposedCandidate(p_site, candidate, band, alongMin, alongMax, alongCenter))
		{
			return std::nullopt;
		}
		return candidate;
	}

	// A long one-pass flight may run out of clear low ground along the cliff:
	// fold taller climbs into two adjacent, opposing runs joined by a 3x6
	// landing before giving up on the edge.
	std::optional<Generator::StairCandidate> Generator::makeSwitchbackCandidate(
		const StairSite &p_site,
		const std::string &p_platformId,
		int p_tangent,
		int p_crossOffset)
	{
		const CliffFrame &frame = p_site.frame;
		const int run = cfg.blocksPerLevel;
		const int steps = p_site.steps;
		const int firstPassSteps = (steps + 1) / 2;
		const int secondPassSteps = steps - firstPassSteps;
		const int middleStand = p_site.surface + 1 + run * secondPassSteps;
		const int nearCenter = p_site.wallColumn + p_site.wallSide;
		const int farCenter = nearCenter + 3 * p_site.wallSide;
		const int bandNearColumn = p_site.wallColumn + 6 * p_site.wallSide;
		const int bandFarColumn = p_site.wallColumn + 8 * p_site.wallSide;
		const int topAlong = p_site.alongCenterBase + p_crossOffset;
		const int turnAlong = topAlong + p_tangent * run * (firstPassSteps + 1);
		const int bottomAlong = turnAlong - p_tangent * run * (secondPassSteps + 1);

		StairCandidate candidate;
		candidate.record.layout = StairLayout::Switchback;
		candidate.record.topAnchor = frame.at(nearCenter, p_site.highSurface + 1, topAlong);
		candidate.record.bottomAnchor = frame.at(farCenter, p_site.surface + 1, bottomAlong);
		candidate.placements.reserve(static_cast<std::size_t>(steps) + 4);
		auto addPlatform = [&](int p_across, int p_y, int p_along) {
			PrefabPlacement platform;
			platform.prefabId = p_platformId;
			platform.foundation = true;
			platform.anchor = frame.at(p_across, p_y, p_along);
			candidate.placements.push_back(std::move(platform));
		};
		auto addFlight = [&](int p_across, int p_y, int p_along, int p_ascendTangent) {
			PrefabPlacement piece;
			piece.prefabId = pickStairLength(p_site.lowZone, p_site.onRoad);
			piece.orientation = frame.alongAscend(p_ascendTangent);
			piece.foundation = true;
			piece.anchor = frame.at(p_across, p_y, p_along);
			candidate.placements.push_back(std::move(piece));
		};

		addPlatform(nearCenter, p_site.highSurface + 1, topAlong);
		for (int segment = 1; segment <= firstPassSteps; ++segment)
		{
			addFlight(
				nearCenter,
				p_site.surface + 1 + run * (steps - segment),
				topAlong + p_tangent * run * segment,
				p_tangent);
		}
		addPlatform(nearCenter, middleStand, turnAlong);
		addPlatform(farCenter, middleStand, turnAlong);
		for (int segment = 1; segment <= secondPassSteps; ++segment)
		{
			addFlight(
				farCenter,
				p_site.surface + 1 + run * (secondPassSteps - segment),
				turnAlong - p_tangent * run * segment,
				-p_tangent);
		}
		addPlatform(farCenter, p_site.surface + 1, bottomAlong);

		const int alongMin = std::min({topAlong, turnAlong, bottomAlong}) - 1;
		const int alongMax = std::max({topAlong, turnAlong, bottomAlong}) + 1;
		const StairFootprint band = frame.rect(
			std::min(bandNearColumn, bandFarColumn), std::max(bandNearColumn, bandFarColumn), alongMin, alongMax);

		candidate.record.centerPath.reserve(
			static_cast<std::size_t>(run * (firstPassSteps + secondPassSteps + 2) + 4));
		for (int distance = 0; distance <= run * (firstPassSteps + 1); ++distance)
		{
			candidate.record.centerPath.push_back(frame.at(nearCenter, 0, topAlong + p_tangent * distance));
		}
		for (int laneStep = 1; laneStep <= 3; ++laneStep)
		{
			candidate.record.centerPath.push_back(
				frame.at(nearCenter + p_site.wallSide * laneStep, 0, turnAlong));
		}
		for (int distance = 1; distance <= run * (secondPassSteps + 1); ++distance)
		{
			candidate.record.centerPath.push_back(frame.at(farCenter, 0, turnAlong - p_tangent * distance));
		}
		if (!guardComposedCandidate(p_site, candidate, band, alongMin, alongMax, topAlong))
		{
			return std::nullopt;
		}
		return candidate;
	}

	// A centered straight flight crossing away from the cliff: the shape of
	// every one-level ramp, and the last-resort fallback for taller climbs.
	Generator::StairCandidate Generator::makePerpendicularCandidate(const StairSite &p_site)
	{
		const CliffFrame &frame = p_site.frame;
		const int run = cfg.blocksPerLevel;
		const int steps = p_site.steps;
		const int alongCenter = p_site.alongCenterBase;

		StairCandidate candidate;
		candidate.record.layout = StairLayout::Perpendicular;
		candidate.placements.reserve(static_cast<std::size_t>(steps));
		for (int segment = 1; segment <= steps; ++segment)
		{
			PrefabPlacement placement;
			// Each segment draws its own flight variant so a climb reads as a mix.
			placement.prefabId = pickStairLength(p_site.lowZone, p_site.onRoad);
			placement.foundation = true;
			const int rise = p_site.surface + 1 + run * (segment - 1);
			const int distanceFromBoundary = run * (steps - segment); // 0 = against the boundary
			placement.orientation = frame.acrossAscend(p_site.firstLower);
			const int minAcross = p_site.firstLower
									  ? p_site.boundary - distanceFromBoundary - run
									  : p_site.boundary + distanceFromBoundary;
			placement.anchor = frame.at(minAcross + run / 2, rise, alongCenter);
			candidate.placements.push_back(std::move(placement));
		}
		const int topAcross = p_site.wallColumn;
		const int descent = p_site.wallSide;
		const int bottomAcross = topAcross + descent * (run * steps + 1);
		candidate.record.topAnchor = frame.at(topAcross, p_site.highSurface + 1, alongCenter);
		candidate.record.bottomAnchor = frame.at(bottomAcross, p_site.surface + 1, alongCenter);
		candidate.record.plateauCell = frame.at(topAcross - descent, 0, alongCenter);
		candidate.record.centerPath.reserve(static_cast<std::size_t>(run * steps + 2));
		for (int distance = 0; distance <= run * steps + 1; ++distance)
		{
			candidate.record.centerPath.push_back(frame.at(topAcross + descent * distance, 0, alongCenter));
		}
		return candidate;
	}

	// Places an adaptive staircase across a cliff. One-level climbs use a
	// centered perpendicular ramp. Taller climbs try, in order, one run along
	// the cliff, a compact two-run switchback, and finally a perpendicular
	// chain; both tangent directions and three sideways nudges are tried per
	// composed layout. Every committed staircase becomes a PlanStairway record.
	// Returns the number of emitted prefab placements.
	int Generator::emitStairChain(
		int p_row,
		int p_col,
		int p_dr,
		int p_dc,
		bool p_onRoad,
		std::optional<int> p_wildMaximumLevels)
	{
		const int blocks = cfg.blocksPerCell;
		const int offset = plan.worldOffset();
		const int strip = blocks / 2 - 1; // strip start (3-wide, centered)
		const CliffFrame frame{.acrossIsX = p_dc == 1, .plan = plan};
		const int nr = p_row + p_dr;
		const int nc = p_col + p_dc;
		const int levelA = plan.height.at(p_row, p_col);
		const int levelB = plan.height.at(nr, nc);
		if (levelA == levelB)
		{
			return 0;
		}
		const bool firstLower = levelA < levelB;
		const int lowRow = firstLower ? p_row : nr;
		const int lowCol = firstLower ? p_col : nc;
		const int lowLevel = std::min(levelA, levelB);
		const int highLevel = std::max(levelA, levelB);
		const int steps = highLevel - lowLevel;

		const int maximumLevels = p_onRoad ? cfg.maxComposedStairLevels : p_wildMaximumLevels.value_or(cfg.maxWildStairLevels);
		if (steps > maximumLevels)
		{
			++plan.stats.rejectedStairways;
			return 0;
		}

		const int boundary = frame.acrossIsX ? offset + (p_col + 1) * blocks
											 : offset + (p_row + 1) * blocks;
		const StairSite site{
			.frame = frame,
			.steps = steps,
			.lowLevel = lowLevel,
			.highLevel = highLevel,
			.lowZone = plan.zone.at(lowRow, lowCol),
			.lowRow = lowRow,
			.lowCol = lowCol,
			.surface = plan.surfaceY(lowLevel),
			.highSurface = plan.surfaceY(highLevel),
			.boundary = boundary,
			.wallSide = firstLower ? -1 : 1, // away from the wall, low side
			.wallColumn = firstLower ? boundary - 1 : boundary,
			.alongCenterBase = (frame.acrossIsX ? offset + p_row * blocks : offset + p_col * blocks) + strip + 1,
			.firstLower = firstLower,
			.onRoad = p_onRoad};

		const auto commit = [&](std::optional<StairCandidate> p_candidate) {
			return p_candidate.has_value() && tryCommitStairCandidate(site, std::move(*p_candidate));
		};

		if (steps >= 2)
		{
			const std::string platformId = stairPlatformPrefabFor(site.lowZone, p_onRoad);
			constexpr std::array tangents = {1, -1};
			constexpr std::array crossOffsets = {0, -1, 1};
			for (const int tangent : tangents)
			{
				for (const int crossOffset : crossOffsets)
				{
					if (commit(makeOnePassCandidate(site, platformId, tangent, crossOffset)))
					{
						return static_cast<int>(plan.stairways.back().placementCount);
					}
				}
			}
			for (const int tangent : tangents)
			{
				for (const int crossOffset : crossOffsets)
				{
					if (commit(makeSwitchbackCandidate(site, platformId, tangent, crossOffset)))
					{
						return static_cast<int>(plan.stairways.back().placementCount);
					}
				}
			}
			// Last resort: a short transition may still fit as a centered straight
			// flight perpendicular to the cliff.
			if (p_onRoad && steps > cfg.maxRoadStairLevels)
			{
				++plan.stats.rejectedStairways;
				return 0;
			}
		}
		if (!commit(makePerpendicularCandidate(site)))
		{
			++plan.stats.rejectedStairways;
			return 0;
		}
		return static_cast<int>(plan.stairways.back().placementCount);
	}

	// Stairways: wherever the road steps between two strata levels, place an
	// adaptive staircase sized to the complete height difference.
	void Generator::placeStairways()
	{
		for (int i = 0; i < size; ++i)
		{
			for (int j = 0; j < size; ++j)
			{
				if (plan.road.at(i, j) == 0 || !isLand(i, j) || plan.bridge.at(i, j) != 0)
				{
					continue;
				}
				for (const auto &[dr, dc] : {std::pair{0, 1}, {1, 0}})
				{
					const int nr = i + dr;
					const int nc = j + dc;
					if (nr >= size || nc >= size || plan.road.at(nr, nc) == 0 || !isLand(nr, nc) ||
						plan.bridge.at(nr, nc) != 0)
					{
						continue;
					}
					plan.stats.stairPlacements += emitStairChain(i, j, dr, dc, true);
				}
			}
		}
	}

	// Wild staircases: off-road ramps across cliffs inside a zone, so exploration
	// is not funneled onto roads. Candidates avoid roads, water and entities and
	// keep their distance from every other stairway.
	void Generator::placeWildStairways()
	{
		// Composed staircases climb any wild cliff up to the configured cap, or all
		// accepted candidates for a biome override without maxPerZone. The candidate
		// scan only proposes edges the composer can actually serve.
		std::set<std::pair<int, int>> entityCells;
		for (const PlanEntity &entity : plan.entities)
		{
			entityCells.insert({entity.row, entity.col});
		}
		struct Candidate
		{
			int row;
			int col;
			int dr;
			int dc;
		};
		for (const PlanZone &zone : plan.zones)
		{
			const PlanBiome &biome = plan.biomes[zone.biomeIndex];
			const std::optional<int> maxPerZone =
				biome.wildStairsConfigured ? biome.wildStairsMaxPerZone : std::optional<int>{cfg.wildStairsPerZone};
			if (maxPerZone.has_value() && *maxPerZone <= 0)
			{
				continue;
			}
			const double spacingCells = biome.wildStairSpacingCells.value_or(cfg.wildStairSpacingCells);
			const double candidateRatio = biome.wildStairCandidateRatio.value_or(1.0);
			const bool allowCrossZone = biome.wildStairAllowCrossZone.value_or(false);
			const int maxLevels = std::min(
				{biome.wildStairMaxLevels.value_or(cfg.maxWildStairLevels),
				 cfg.maxComposedStairLevels,
				 cfg.maxHeightLevel});
			Rng rng = rngFor("zone:" + std::to_string(zone.id) + "/wild_stairs");
			std::vector<Candidate> candidates;
			const auto usable = [&](int p_row, int p_col) {
				return plan.zone.contains(p_row, p_col) && isLand(p_row, p_col) && plan.road.at(p_row, p_col) == 0 &&
					   plan.water.at(p_row, p_col) == 0 && !entityCells.contains({p_row, p_col});
			};
			for (int i = 0; i < size; ++i)
			{
				for (int j = 0; j < size; ++j)
				{
					if (!usable(i, j))
					{
						continue;
					}
					for (const auto &[dr, dc] : {std::pair{0, 1}, {1, 0}})
					{
						if (!usable(i + dr, j + dc))
						{
							continue;
						}
						const int dh = std::abs(plan.height.at(i, j) - plan.height.at(i + dr, j + dc));
						if (dh >= 1 && dh <= maxLevels)
						{
							const bool firstLower = plan.height.at(i, j) < plan.height.at(i + dr, j + dc);
							const int lowRow = firstLower ? i : i + dr;
							const int lowCol = firstLower ? j : j + dc;
							const int highRow = firstLower ? i + dr : i;
							const int highCol = firstLower ? j + dc : j;
							if (plan.zone.at(lowRow, lowCol) == zone.id &&
								(allowCrossZone || plan.zone.at(highRow, highCol) == zone.id))
							{
								candidates.push_back({i, j, dr, dc});
							}
						}
					}
				}
			}
			plan.stats.wildStairCandidates += static_cast<int>(candidates.size());
			rng.shuffle(candidates);
			int placed = 0;
			for (const Candidate &candidate : candidates)
			{
				if (maxPerZone.has_value() && placed >= *maxPerZone)
				{
					break;
				}
				if (candidateRatio < 1.0 && rng.uniform() >= candidateRatio)
				{
					++plan.stats.wildStairRatioSkips;
					continue;
				}
				const bool tooClose = std::any_of(stairCells.begin(), stairCells.end(), [&](const Cell &p_cell) {
					return std::max(std::abs(p_cell.row - candidate.row), std::abs(p_cell.col - candidate.col)) <
						   spacingCells;
				});
				if (tooClose)
				{
					++plan.stats.wildStairSpacingSkips;
					continue;
				}
				const int segments =
					emitStairChain(candidate.row, candidate.col, candidate.dr, candidate.dc, false, maxLevels);
				if (segments > 0)
				{
					plan.stats.wildStairPlacements += segments;
					++placed;
				}
				else
				{
					++plan.stats.wildStairPlacementRejects;
				}
			}
		}
	}
}
