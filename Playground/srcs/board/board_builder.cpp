#include "board/board_builder.hpp"

#include "core/game_rules.hpp"
#include "world/voxel_world.hpp"

#include "structures/voxel/spk_voxel_chunk.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <set>
#include <string>
#include <utility>

namespace pg
{
	namespace
	{
		// The candidate offsets, in the one order every live build inspects them in. The primary
		// anchor first, then the four axis nudges, then the four diagonals - two cells at a time, so a
		// rejected anchor moves the board rather than jittering it.
		constexpr std::array<spk::Vector2Int, 9> CandidateOffsets = {
			spk::Vector2Int{0, 0},
			spk::Vector2Int{2, 0},
			spk::Vector2Int{-2, 0},
			spk::Vector2Int{0, 2},
			spk::Vector2Int{0, -2},
			spk::Vector2Int{2, 2},
			spk::Vector2Int{-2, 2},
			spk::Vector2Int{2, -2},
			spk::Vector2Int{-2, -2}};

		[[nodiscard]] BoardBuildResult failed(BoardBuildError p_error)
		{
			return BoardBuildResult{.board = std::nullopt, .error = std::move(p_error)};
		}

		[[nodiscard]] BoardBuildError invalidRequest(std::string p_detail)
		{
			return BoardBuildError{
				.code = BoardBuildErrorCode::InvalidRequest, .diagnosticDetail = std::move(p_detail)};
		}

		[[nodiscard]] bool isValidGap(double p_maxVerticalGap) noexcept
		{
			return std::isfinite(p_maxVerticalGap) && p_maxVerticalGap >= 0.0;
		}

		// The board's border ring: the standable nodes sitting on the outer rectangle. Solid boundary
		// voxels nobody can stand on are not border cells - a wall is terrain, not an edge.
		[[nodiscard]] std::vector<BoardCell> collectBorderCells(
			const TraversalGraph &p_navigation,
			const spk::Vector2Int &p_size)
		{
			std::vector<BoardCell> result;
			for (const TraversalGraph::Node &node : p_navigation.allNodes())
			{
				if (node.position.x == 0 || node.position.x == p_size.x - 1 || node.position.z == 0 ||
					node.position.z == p_size.y - 1)
				{
					result.push_back(node.position);
				}
			}
			std::ranges::sort(result, BoardCellLess{});
			return result;
		}

		// How many standable support cells one strip offers, scanned straight off the cells. Candidate
		// inspection deliberately does not build a TraversalGraph per candidate: eight of the nine
		// would be thrown away, and the final graph is built once, for the anchor that won.
		[[nodiscard]] std::size_t countStandableInStrip(
			const ICellSource &p_source,
			const spk::Vector2Int &p_size,
			int p_minimumY,
			int p_maximumY,
			bool p_axisIsZ,
			int p_stripMinimum,
			int p_stripMaximum)
		{
			std::size_t count = 0;
			for (int z = 0; z < p_size.y; ++z)
			{
				for (int x = 0; x < p_size.x; ++x)
				{
					const int axis = p_axisIsZ ? z : x;
					if (axis < p_stripMinimum || axis > p_stripMaximum)
					{
						continue;
					}
					for (int y = p_minimumY; y < p_maximumY; ++y)
					{
						if (isStandable(p_source, spk::Vector3Int{x, y, z}))
						{
							++count;
						}
					}
				}
			}
			return count;
		}
	}

	WorldBoardPlanResult BoardBuilder::planWorld(const WorldBoardRequest &p_request)
	{
		if (p_request.size.x < BattleGameRules::MinimumBoardSide ||
			p_request.size.x > BattleGameRules::MaximumBoardSide ||
			p_request.size.y < BattleGameRules::MinimumBoardSide ||
			p_request.size.y > BattleGameRules::MaximumBoardSide)
		{
			return {
				.plan = std::nullopt,
				.error = invalidRequest(
					"a live board is " + std::to_string(BattleGameRules::MinimumBoardSide) + " to " +
					std::to_string(BattleGameRules::MaximumBoardSide) + " cells on each horizontal axis, got " +
					p_request.size.toString())};
		}
		if (!isHorizontalApproach(p_request.approachDirection))
		{
			return {.plan = std::nullopt, .error = invalidRequest("the approach into an encounter is horizontal")};
		}
		if (p_request.minimumWorldY >= p_request.maximumWorldY)
		{
			return {
				.plan = std::nullopt,
				.error = invalidRequest(
					"the navigation Y bounds are a non-empty half-open interval, got [" +
					std::to_string(p_request.minimumWorldY) + ", " + std::to_string(p_request.maximumWorldY) + ")")};
		}
		if (p_request.encounterSupportCell.y < p_request.minimumWorldY ||
			p_request.encounterSupportCell.y >= p_request.maximumWorldY)
		{
			return {
				.plan = std::nullopt,
				.error = invalidRequest(
					"the encounter support cell " + p_request.encounterSupportCell.toString() +
					" is outside the navigation Y bounds that made it reachable")};
		}

		const int approachExtent =
			approachAxisIsZ(p_request.approachDirection) ? p_request.size.y : p_request.size.x;
		if (p_request.deploymentDepth <= 0 || 2 * p_request.deploymentDepth > approachExtent)
		{
			return {
				.plan = std::nullopt,
				.error = invalidRequest(
					"deployment depth " + std::to_string(p_request.deploymentDepth) + " does not fit twice in the " +
					std::to_string(approachExtent) + "-row approach axis")};
		}

		// The board is centred on the cell the encounter triggered on. An even side has no centre
		// column, so the anchor lands half a cell "before" it - which is what floor(size / 2) means for
		// the strictly positive sizes above.
		const std::optional<int> primaryX = trySubtract(p_request.encounterSupportCell.x, p_request.size.x / 2);
		const std::optional<int> primaryZ = trySubtract(p_request.encounterSupportCell.z, p_request.size.y / 2);
		// The two head-clearance cells above the top scanned support cell are read by standability, so
		// they are part of the region that has to be resident.
		const std::optional<int> clearanceTopY = tryAdd(p_request.maximumWorldY, 1);
		if (!primaryX.has_value() || !primaryZ.has_value() || !clearanceTopY.has_value())
		{
			return {
				.plan = std::nullopt,
				.error = BoardBuildError{
					.code = BoardBuildErrorCode::NumericOverflow,
					.diagnosticDetail = "the primary anchor or the head-clearance ceiling overflows the cell space"}};
		}

		WorldBoardPlan plan;
		plan.request = p_request;

		std::set<spk::Vector3Int, CellPositionLess> requiredChunks;
		for (const spk::Vector2Int &offset : CandidateOffsets)
		{
			const std::optional<int> anchorX = tryAdd(*primaryX, offset.x);
			const std::optional<int> anchorZ = tryAdd(*primaryZ, offset.y);
			if (!anchorX.has_value() || !anchorZ.has_value())
			{
				continue;
			}
			// The far corner has to exist too: a board whose last column wraps is not a board.
			const std::optional<int> lastX = tryAdd(*anchorX, p_request.size.x - 1);
			const std::optional<int> lastZ = tryAdd(*anchorZ, p_request.size.y - 1);
			if (!lastX.has_value() || !lastZ.has_value())
			{
				continue;
			}

			plan.candidateWorldAnchors.push_back(spk::Vector3Int{*anchorX, 0, *anchorZ});

			// The chunk box that covers this candidate's whole rectangle, its whole Y scan range, and
			// the head clearance above it. spk::VoxelChunk owns the negative-coordinate division rule;
			// nothing here re-derives it.
			const spk::Vector3Int minimumChunk = spk::VoxelChunk::coordinatesFromWorldCell(
				{*anchorX, p_request.minimumWorldY, *anchorZ});
			const spk::Vector3Int maximumChunk =
				spk::VoxelChunk::coordinatesFromWorldCell({*lastX, *clearanceTopY, *lastZ});
			for (int chunkY = minimumChunk.y; chunkY <= maximumChunk.y; ++chunkY)
			{
				for (int chunkZ = minimumChunk.z; chunkZ <= maximumChunk.z; ++chunkZ)
				{
					for (int chunkX = minimumChunk.x; chunkX <= maximumChunk.x; ++chunkX)
					{
						requiredChunks.insert(spk::Vector3Int{chunkX, chunkY, chunkZ});
					}
				}
			}
		}

		if (plan.candidateWorldAnchors.empty() || requiredChunks.empty())
		{
			return {
				.plan = std::nullopt,
				.error = BoardBuildError{
					.code = BoardBuildErrorCode::NumericOverflow,
					.diagnosticDetail = "every candidate anchor overflows the cell space"}};
		}

		plan.requiredChunkCoordinates.assign(requiredChunks.begin(), requiredChunks.end());
		const spk::Vector3Int firstChunk = *requiredChunks.begin();
		spk::Vector3Int minimumChunk = firstChunk;
		spk::Vector3Int maximumChunk = firstChunk;
		for (const spk::Vector3Int &coordinates : requiredChunks)
		{
			minimumChunk.x = std::min(minimumChunk.x, coordinates.x);
			minimumChunk.y = std::min(minimumChunk.y, coordinates.y);
			minimumChunk.z = std::min(minimumChunk.z, coordinates.z);
			maximumChunk.x = std::max(maximumChunk.x, coordinates.x);
			maximumChunk.y = std::max(maximumChunk.y, coordinates.y);
			maximumChunk.z = std::max(maximumChunk.z, coordinates.z);
		}
		plan.pinWindowOriginChunk = minimumChunk;
		plan.pinWindowRange = {
			maximumChunk.x - minimumChunk.x + 1,
			maximumChunk.y - minimumChunk.y + 1,
			maximumChunk.z - minimumChunk.z + 1};
		return {.plan = std::move(plan), .error = std::nullopt};
	}

	BoardBuildResult BoardBuilder::buildWorld(
		const VoxelWorld &p_world,
		const WorldBoardPlan &p_plan,
		double p_maxVerticalGap)
	{
		if (!isValidGap(p_maxVerticalGap))
		{
			return failed(invalidRequest("the maximum vertical traversal gap is a finite non-negative value"));
		}
		if (p_plan.candidateWorldAnchors.empty() || p_plan.requiredChunkCoordinates.empty())
		{
			return failed(invalidRequest("this plan has no candidate anchor: it was never produced by planWorld"));
		}

		// Every required chunk has to be resident and active before a single cell is read: a board
		// built over a hole the streamer has not filled yet would be a board over fabricated air.
		const spk::VoxelMap &map = p_world.map();
		std::vector<RequiredChunkStamp> stamps;
		stamps.reserve(p_plan.requiredChunkCoordinates.size());
		for (const spk::Vector3Int &coordinates : p_plan.requiredChunkCoordinates)
		{
			const spk::VoxelChunk *chunk = map.tryChunk(coordinates);
			if (chunk == nullptr || !chunk->isActivated())
			{
				return failed(BoardBuildError{.code = BoardBuildErrorCode::RequiredChunkUnavailable, .cell = coordinates, .diagnosticDetail = chunk == nullptr ? "required chunk " + coordinates.toString() + " is not loaded" : "required chunk " + coordinates.toString() + " is inactive"});
			}
			stamps.push_back({.coordinates = coordinates, .contentRevision = chunk->contentRevision()});
		}

		const WorldBoardRequest &request = p_plan.request;
		const DeploymentStrips strips =
			deploymentStrips(request.size, request.approachDirection, request.deploymentDepth);

		std::string candidateReport;
		std::size_t bestPlayerSeats = 0;
		std::size_t bestEnemySeats = 0;
		for (const spk::Vector3Int &anchor : p_plan.candidateWorldAnchors)
		{
			const WorldCellSource candidate(p_world, anchor);
			const std::size_t playerSeats = countStandableInStrip(
				candidate,
				request.size,
				request.minimumWorldY,
				request.maximumWorldY,
				strips.axisIsZ,
				strips.playerMinimum,
				strips.playerMaximum);
			const std::size_t enemySeats = countStandableInStrip(
				candidate,
				request.size,
				request.minimumWorldY,
				request.maximumWorldY,
				strips.axisIsZ,
				strips.enemyMinimum,
				strips.enemyMaximum);

			if (playerSeats >= request.playerSeatCount && enemySeats >= request.enemySeatCount)
			{
				const TraversalBounds bounds{
					.minimum = {0, request.minimumWorldY, 0},
					.maximum = {request.size.x, request.maximumWorldY, request.size.y}};
				return buildFrozenSource(
					BoardSourceDescriptor{.kind = BoardSourceKind::LiveWorld, .definitionId = std::nullopt},
					std::make_shared<const WorldCellSource>(p_world, anchor),
					BoardExtent{.size = request.size, .traversalBounds = bounds},
					anchor,
					anchor,
					LiveBoardTerrainStamp{
						.world = &p_world, .mapLifetime = map.lifetimeToken(), .requiredChunks = std::move(stamps)},
					request.approachDirection,
					request.deploymentDepth,
					request.playerSeatCount,
					request.enemySeatCount,
					p_maxVerticalGap);
			}

			bestPlayerSeats = std::max(bestPlayerSeats, playerSeats);
			bestEnemySeats = std::max(bestEnemySeats, enemySeats);
			candidateReport += (candidateReport.empty() ? "" : "; ") + anchor.toString() + " seats " +
							   std::to_string(playerSeats) + "/" + std::to_string(enemySeats);
		}

		// No shrinking, no dropped creature, no fabricated ground: the encounter is declined instead.
		return failed(BoardBuildError{.code = BoardBuildErrorCode::NoFeasibleWorldAnchor, .requiredPlayerSeats = request.playerSeatCount, .availablePlayerSeats = bestPlayerSeats, .requiredEnemySeats = request.enemySeatCount, .availableEnemySeats = bestEnemySeats, .diagnosticDetail = "no candidate anchor seats both teams (player/enemy per candidate: " + candidateReport + ")"});
	}

	BoardBuildResult BoardBuilder::buildHandcrafted(
		const HandcraftedBattleBoardDefinition &p_definition,
		const PrefabDefinition &p_geometry,
		const VoxelRegistry &p_voxels,
		std::size_t p_playerSeatCount,
		std::size_t p_enemySeatCount,
		double p_maxVerticalGap)
	{
		const auto invalidDefinition = [&p_definition, &p_geometry](std::string p_detail) {
			return failed(BoardBuildError{.code = BoardBuildErrorCode::InvalidHandcraftedDefinition, .boardOrEncounterId = p_definition.id, .prefabId = p_geometry.id, .diagnosticDetail = std::move(p_detail)});
		};

		// The registry already validated all of this. It is rechecked anyway, because the next thing
		// this function does is allocate a grid from these numbers, and an allocator is never handed a
		// product nobody looked at.
		if (!isValidGap(p_maxVerticalGap))
		{
			return invalidDefinition("the maximum vertical traversal gap is a finite non-negative value");
		}
		if (p_definition.geometryPrefabId != p_geometry.id)
		{
			return invalidDefinition(
				"board '" + p_definition.id + "' names geometry prefab '" + p_definition.geometryPrefabId +
				"' but was handed '" + p_geometry.id + "'");
		}
		const spk::Vector3Int size = p_definition.size;
		if (size.x < MinimumBattleBoardSide || size.x > MaximumBattleBoardSide || size.z < MinimumBattleBoardSide ||
			size.z > MaximumBattleBoardSide || size.x % 2 == 0 || size.z % 2 == 0 || size.y < MinimumBattleBoardHeight ||
			size.y > MaximumBattleBoardHeight)
		{
			return invalidDefinition("the board size " + size.toString() + " is outside the arena schema");
		}
		const std::int64_t volume = static_cast<std::int64_t>(size.x) * static_cast<std::int64_t>(size.y) *
									static_cast<std::int64_t>(size.z);
		constexpr std::int64_t MaximumVolume = static_cast<std::int64_t>(MaximumBattleBoardSide) *
											   static_cast<std::int64_t>(MaximumBattleBoardSide) *
											   static_cast<std::int64_t>(MaximumBattleBoardHeight);
		if (volume <= 0 || volume > MaximumVolume)
		{
			return invalidDefinition("the board holds " + std::to_string(volume) + " cells, more than an arena may allocate");
		}
		if (!isHorizontalApproach(p_definition.playerApproach))
		{
			return invalidDefinition("the player approach into an arena is horizontal");
		}
		const spk::Vector2Int extentSize{size.x, size.z};
		const int approachExtent = approachAxisExtent(p_definition);
		if (p_definition.deploymentDepth <= 0 || 2 * p_definition.deploymentDepth > approachExtent)
		{
			return invalidDefinition(
				"deployment depth " + std::to_string(p_definition.deploymentDepth) + " does not fit twice in the " +
				std::to_string(approachExtent) + "-row approach axis");
		}

		// The identity transform: destination == pivot and PositiveZ, so an authored prefab coordinate
		// IS a board-local coordinate - no rotation, no flip, no translation.
		std::vector<std::pair<spk::Vector3Int, spk::VoxelCell>> applied;
		applied.reserve(p_geometry.prefab.voxels().size());
		p_geometry.prefab.forEachAppliedVoxel(
			p_geometry.prefab.pivot(),
			VoxelOrientation::PositiveZ,
			[&applied](const spk::Vector3Int &p_position, const spk::VoxelCell &p_cell) {
				applied.emplace_back(p_position, p_cell);
			});

		// Every applied position is checked - carve entries included - before one cell is written.
		// Prefab::applyTo would clip an outside voxel away, and an arena silently missing a wall is
		// worse than one that refuses to load.
		for (const auto &[position, cell] : applied)
		{
			const bool inside = position.x >= 0 && position.x < size.x && position.y >= 0 && position.y < size.y &&
								position.z >= 0 && position.z < size.z;
			if (!inside)
			{
				return failed(BoardBuildError{.code = BoardBuildErrorCode::PrefabVoxelOutOfBounds, .boardOrEncounterId = p_definition.id, .prefabId = p_geometry.id, .cell = position, .diagnosticDetail = "prefab '" + p_geometry.id + "' places a voxel at " + position.toString() + ", outside board '" + p_definition.id + "' and its [0, " + size.toString() + ") box"});
			}
		}

		const auto grid = std::make_shared<spk::VoxelGrid>(size);
		for (const auto &[position, cell] : applied)
		{
			grid->cell(position) = cell;
		}

		const TraversalBounds bounds{.minimum = {0, 0, 0}, .maximum = size};
		return buildFrozenSource(
			BoardSourceDescriptor{.kind = BoardSourceKind::Handcrafted, .definitionId = p_definition.id},
			std::make_shared<const GridCellSource>(std::shared_ptr<const spk::VoxelGrid>(grid), p_voxels),
			BoardExtent{.size = extentSize, .traversalBounds = bounds},
			spk::Vector3Int{},
			std::nullopt,
			std::nullopt,
			p_definition.playerApproach,
			p_definition.deploymentDepth,
			p_playerSeatCount,
			p_enemySeatCount,
			p_maxVerticalGap);
	}

	BoardBuildResult BoardBuilder::buildFrozenSource(
		BoardSourceDescriptor p_source,
		std::shared_ptr<const ICellSource> p_cells,
		BoardExtent p_extent,
		spk::Vector3Int p_presentationOrigin,
		std::optional<spk::Vector3Int> p_liveWorldAnchor,
		std::optional<LiveBoardTerrainStamp> p_liveTerrainStamp,
		VoxelOrientation p_playerApproach,
		int p_deploymentDepth,
		std::size_t p_playerSeatCount,
		std::size_t p_enemySeatCount,
		double p_maxVerticalGap)
	{
		const std::string boardId = p_source.definitionId.value_or("liveWorld");
		if (p_cells == nullptr)
		{
			return failed(invalidRequest("a board is built over a cell source"));
		}
		if (!isValidGap(p_maxVerticalGap))
		{
			return failed(invalidRequest("the maximum vertical traversal gap is a finite non-negative value"));
		}
		if (!isHorizontalApproach(p_playerApproach) || p_extent.size.x <= 0 || p_extent.size.y <= 0)
		{
			return failed(invalidRequest("a board has a positive horizontal extent and a horizontal approach"));
		}

		const int approachExtent = approachAxisIsZ(p_playerApproach) ? p_extent.size.y : p_extent.size.x;
		if (p_deploymentDepth <= 0 || 2 * p_deploymentDepth > approachExtent)
		{
			return failed(BoardBuildError{.code = BoardBuildErrorCode::InvalidRequest, .boardOrEncounterId = boardId, .diagnosticDetail = "deployment depth " + std::to_string(p_deploymentDepth) + " does not fit twice in the " + std::to_string(approachExtent) + "-row approach axis"});
		}

		// The one graph. Built here, once, from the frozen source - and never refreshed again for as
		// long as this board exists.
		TraversalGraph navigation = TraversalGraphBuilder::build(
			*p_cells, p_extent.traversalBounds, static_cast<float>(p_maxVerticalGap));
		if (navigation.size() == 0)
		{
			return failed(BoardBuildError{.code = BoardBuildErrorCode::TraversalBuildFailed, .boardOrEncounterId = boardId, .requiredPlayerSeats = p_playerSeatCount, .requiredEnemySeats = p_enemySeatCount, .diagnosticDetail = "no cell of this board can be stood on"});
		}

		DeploymentLayout deployment =
			buildDeploymentLayout(navigation, p_extent.size, p_playerApproach, p_deploymentDepth);
		if (deployment.playerCells.size() < p_playerSeatCount || deployment.enemyCells.size() < p_enemySeatCount)
		{
			// A pretty but unplayable arena is invalid. Nothing is dropped, overlapped or invented.
			return failed(BoardBuildError{.code = BoardBuildErrorCode::InsufficientDeploymentCapacity, .boardOrEncounterId = boardId, .requiredPlayerSeats = p_playerSeatCount, .availablePlayerSeats = deployment.playerCells.size(), .requiredEnemySeats = p_enemySeatCount, .availableEnemySeats = deployment.enemyCells.size(), .diagnosticDetail = "the deployment strips cannot seat both teams"});
		}

		std::vector<BoardCell> borders = collectBorderCells(navigation, p_extent.size);

		BoardData::Parts parts{
			.source = std::move(p_source),
			.cells = std::move(p_cells),
			.presentationOrigin = p_presentationOrigin,
			.liveWorldAnchor = p_liveWorldAnchor,
			.extent = p_extent,
			.navigation = std::move(navigation),
			.deployment = std::move(deployment),
			.borderCells = std::move(borders),
			.liveTerrainStamp = std::move(p_liveTerrainStamp)};
		return BoardBuildResult{.board = BoardData(std::move(parts)), .error = std::nullopt};
	}
}
