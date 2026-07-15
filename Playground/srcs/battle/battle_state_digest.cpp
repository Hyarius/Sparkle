#include "battle/battle_state_digest.hpp"

#include "battle/battle_context.hpp"
#include "core/deterministic_random.hpp"

#include "structures/voxel/spk_voxel_cell.hpp"

#include <algorithm>
#include <cstdint>
#include <vector>

namespace pg
{
	namespace
	{
		using spk::deterministic::StableHasher64;

		void mixU64(StableHasher64 &p_hasher, std::uint64_t p_value) noexcept
		{
			p_hasher.mix(static_cast<std::int32_t>(p_value & 0xFFFFFFFFU));
			p_hasher.mix(static_cast<std::int32_t>(p_value >> 32U));
		}

		void mixInt(StableHasher64 &p_hasher, long long p_value) noexcept
		{
			mixU64(p_hasher, static_cast<std::uint64_t>(p_value));
		}

		void mixBool(StableHasher64 &p_hasher, bool p_value) noexcept
		{
			p_hasher.mix(static_cast<std::int32_t>(p_value ? 1 : 0));
		}

		void mixVec3(StableHasher64 &p_hasher, const spk::Vector3Int &p_value) noexcept
		{
			p_hasher.mix(static_cast<std::int32_t>(p_value.x));
			p_hasher.mix(static_cast<std::int32_t>(p_value.y));
			p_hasher.mix(static_cast<std::int32_t>(p_value.z));
		}

		void mixString(StableHasher64 &p_hasher, std::string_view p_value) noexcept
		{
			mixU64(p_hasher, p_value.size());
			p_hasher.mix(p_value);
		}

		void mixOptString(StableHasher64 &p_hasher, const std::optional<std::string> &p_value) noexcept
		{
			mixBool(p_hasher, p_value.has_value());
			if (p_value.has_value())
			{
				mixString(p_hasher, *p_value);
			}
		}

		void mixOptCell(StableHasher64 &p_hasher, const std::optional<BoardCell> &p_value) noexcept
		{
			mixBool(p_hasher, p_value.has_value());
			if (p_value.has_value())
			{
				mixVec3(p_hasher, *p_value);
			}
		}

		// The immutable board topology: extent + traversal bounds, every bounded source voxel in
		// X/Y/Z order, the sorted navigation nodes/edges with movement costs, both ordered deployment
		// zones, and the sorted border cells. It never reads a live revision or a pointer.
		void mixTopology(StableHasher64 &p_hasher, const BoardData &p_board)
		{
			const BoardExtent &extent = p_board.extent();
			p_hasher.mix(static_cast<std::int32_t>(extent.size.x));
			p_hasher.mix(static_cast<std::int32_t>(extent.size.y));
			mixVec3(p_hasher, extent.traversalBounds.minimum);
			mixVec3(p_hasher, extent.traversalBounds.maximum);

			const spk::Vector3Int minimum = extent.traversalBounds.minimum;
			const spk::Vector3Int maximum = extent.traversalBounds.maximum; // exclusive
			for (int x = minimum.x; x < maximum.x; ++x)
			{
				for (int y = minimum.y; y < maximum.y; ++y)
				{
					for (int z = minimum.z; z < maximum.z; ++z)
					{
						const spk::Vector3Int position{x, y, z};
						const spk::VoxelCell &cell = p_board.cells().cell(position);
						if (cell.isEmpty())
						{
							continue;
						}
						mixVec3(p_hasher, position);
						const VoxelDefinition *definition = p_board.cells().tryDefinition(cell);
						const VoxelStateDefinition *state = p_board.cells().tryState(cell);
						mixString(p_hasher, definition != nullptr ? std::string_view(definition->id) : std::string_view());
						mixU64(p_hasher, state != nullptr ? static_cast<std::uint64_t>(state->id.value) : 0U);
						p_hasher.mix(static_cast<std::int32_t>(cell.orientation));
						p_hasher.mix(static_cast<std::int32_t>(cell.flip));
					}
				}
			}

			// Navigation nodes sorted by position, each with its movement cost and sorted neighbours.
			std::vector<spk::Vector3Int> nodePositions;
			nodePositions.reserve(p_board.navigation().size());
			for (const TraversalGraph::Node &node : p_board.navigation().allNodes())
			{
				nodePositions.push_back(node.position);
			}
			std::sort(nodePositions.begin(), nodePositions.end(), CellPositionLess{});
			mixU64(p_hasher, nodePositions.size());
			for (const spk::Vector3Int &position : nodePositions)
			{
				mixVec3(p_hasher, position);
				mixInt(p_hasher, p_board.movementCost(position));

				const TraversalGraph::Node *node = p_board.navigation().tryGetNode(position);
				std::vector<spk::Vector3Int> neighbours;
				if (node != nullptr)
				{
					for (const std::optional<std::size_t> &neighbour : node->neighbors)
					{
						if (neighbour.has_value())
						{
							neighbours.push_back(p_board.navigation().node(*neighbour).position);
						}
					}
				}
				std::sort(neighbours.begin(), neighbours.end(), CellPositionLess{});
				mixU64(p_hasher, neighbours.size());
				for (const spk::Vector3Int &neighbour : neighbours)
				{
					mixVec3(p_hasher, neighbour);
				}
			}

			for (const BoardCell &cell : p_board.deployment().playerCells)
			{
				mixVec3(p_hasher, cell);
			}
			mixU64(p_hasher, p_board.deployment().playerCells.size());
			for (const BoardCell &cell : p_board.deployment().enemyCells)
			{
				mixVec3(p_hasher, cell);
			}
			mixU64(p_hasher, p_board.deployment().enemyCells.size());

			std::vector<BoardCell> borders(p_board.borderCells().begin(), p_board.borderCells().end());
			std::sort(borders.begin(), borders.end(), CellPositionLess{});
			mixU64(p_hasher, borders.size());
			for (const BoardCell &cell : borders)
			{
				mixVec3(p_hasher, cell);
			}
		}

		void mixBoardSource(StableHasher64 &p_hasher, const BoardData &p_board)
		{
			const BoardSourceDescriptor &source = p_board.sourceDescriptor();
			p_hasher.mix(static_cast<std::int32_t>(source.kind));
			mixOptString(p_hasher, source.definitionId);

			mixTopology(p_hasher, p_board);

			// The live terrain provenance, or its explicit handcrafted absence. presentationOrigin is
			// never folded as an independent transform: for a live board it equals worldAnchor, which
			// is folded once below under its terrain-provenance meaning.
			if (source.kind == BoardSourceKind::LiveWorld)
			{
				mixBool(p_hasher, p_board.liveWorldAnchor().has_value());
				if (p_board.liveWorldAnchor().has_value())
				{
					mixVec3(p_hasher, *p_board.liveWorldAnchor());
				}
				mixBool(p_hasher, p_board.liveTerrainStamp().has_value());
				if (p_board.liveTerrainStamp().has_value())
				{
					const LiveBoardTerrainStamp &stamp = *p_board.liveTerrainStamp();
					std::vector<RequiredChunkStamp> chunks = stamp.requiredChunks;
					std::sort(chunks.begin(), chunks.end(), [](const RequiredChunkStamp &p_a, const RequiredChunkStamp &p_b) {
						return CellPositionLess{}(p_a.coordinates, p_b.coordinates);
					});
					mixU64(p_hasher, chunks.size());
					for (const RequiredChunkStamp &chunk : chunks)
					{
						mixVec3(p_hasher, chunk.coordinates);
						mixU64(p_hasher, chunk.contentRevision);
					}
				}
			}
			else
			{
				// Handcrafted: explicit absence of both live values; the definition id above still
				// distinguishes two authored arenas that share geometry.
				mixBool(p_hasher, false);
				mixBool(p_hasher, false);
			}
		}
	}

	std::uint64_t gameplayProgressDigest(const BattleContext &p_context)
	{
		StableHasher64 hasher;

		mixBoardSource(hasher, p_context.board());

		hasher.mix(static_cast<std::int32_t>(p_context.phase()));
		hasher.mix(static_cast<std::int32_t>(p_context.outcome()));
		mixBool(hasher, p_context.abortReason().has_value());
		if (p_context.abortReason().has_value())
		{
			hasher.mix(static_cast<std::int32_t>(*p_context.abortReason()));
		}
		mixInt(hasher, p_context.elapsed().ticks());

		mixBool(hasher, p_context.turn().has_value());
		if (p_context.turn().has_value())
		{
			mixU64(hasher, p_context.turn()->value);
		}
		mixBool(hasher, p_context.activeUnit().has_value());
		if (p_context.activeUnit().has_value())
		{
			mixU64(hasher, p_context.activeUnit()->value());
		}

		mixBool(hasher, p_context.sideConfirmed(BattleSide::Player));
		mixBool(hasher, p_context.sideConfirmed(BattleSide::Enemy));

		for (const BattleUnitId id : p_context.allUnitsInOrder())
		{
			const BattleUnit &unit = p_context.unit(id);
			mixU64(hasher, id.value());
			hasher.mix(static_cast<std::int32_t>(unit.side()));
			mixU64(hasher, unit.rosterOrder());
			mixString(hasher, unit.speciesId());
			mixString(hasher, unit.formId());
			mixInt(hasher, unit.health());
			mixInt(hasher, unit.maxHealth());
			mixInt(hasher, unit.actionPoints());
			mixInt(hasher, unit.maxActionPoints());
			mixInt(hasher, unit.movementPoints());
			mixInt(hasher, unit.maxMovementPoints());
			mixInt(hasher, unit.effectiveAttributes().stamina.ticks());
			mixInt(hasher, unit.range());
			mixInt(hasher, unit.turnBarFill().ticks());
			mixInt(hasher, unit.nextActivationPenalty().actionPoints);
			mixInt(hasher, unit.nextActivationPenalty().movementPoints);
			hasher.mix(static_cast<std::int32_t>(unit.removalReason()));
			mixBool(hasher, unit.placed());
			mixOptCell(hasher, p_context.board().occupancy().cellOf(id));
		}

		// The RNG draw count is a material fact: two boards that spent different numbers of draws to
		// reach the same layout are not the same gameplay state.
		mixU64(hasher, static_cast<std::uint64_t>(p_context.rngDrawCount()));

		return hasher.value();
	}
}
