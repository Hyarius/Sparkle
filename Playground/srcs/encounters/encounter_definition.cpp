#include "encounters/encounter_definition.hpp"

#include "core/content_id.hpp"

#include <algorithm>
#include <limits>
#include <set>

namespace pg
{
	namespace
	{
		enum class BoardPolicyKind
		{
			LiveWorld,
			Handcrafted
		};

		enum class OpponentPlacementKind
		{
			Fixed,
			ByLine,
			SeededRandom
		};

		[[nodiscard]] const std::map<std::string, BoardPolicyKind> &boardPolicyKindNames()
		{
			static const std::map<std::string, BoardPolicyKind> names{
				{"handcrafted", BoardPolicyKind::Handcrafted},
				{"liveWorld", BoardPolicyKind::LiveWorld}};
			return names;
		}

		[[nodiscard]] const std::map<std::string, OpponentPlacementKind> &opponentPlacementKindNames()
		{
			static const std::map<std::string, OpponentPlacementKind> names{
				{"byLine", OpponentPlacementKind::ByLine},
				{"fixed", OpponentPlacementKind::Fixed},
				{"seededRandom", OpponentPlacementKind::SeededRandom}};
			return names;
		}

		[[nodiscard]] OpponentPlacementPolicy parsePlacement(JsonReader &p_reader)
		{
			const OpponentPlacementKind kind =
				p_reader.requireEnum<OpponentPlacementKind>("type", opponentPlacementKindNames());

			switch (kind)
			{
			case OpponentPlacementKind::Fixed:
			{
				p_reader.forbidUnknown({"type", "cells"});

				FixedOpponentPlacementPolicy policy;
				policy.cells = p_reader.require<std::vector<std::array<int, 2>>>("cells");
				if (policy.cells.empty())
				{
					throw JsonError(p_reader.file(), p_reader.pathFor("cells"), "a fixed placement authors at least one cell");
				}
				return policy;
			}
			case OpponentPlacementKind::ByLine:
			{
				p_reader.forbidUnknown({"type", "rowsFromEnemyEdge", "order"});

				ByLineOpponentPlacementPolicy policy;
				policy.rowsFromEnemyEdge =
					static_cast<int>(requireIntegerInRange(p_reader, "rowsFromEnemyEdge", 0, MaximumBattleBoardSide));
				policy.order = p_reader.requireEnum<OpponentLineOrder>("order", opponentLineOrderNames());
				return policy;
			}
			case OpponentPlacementKind::SeededRandom:
			{
				p_reader.forbidUnknown({"type"});
				return SeededRandomOpponentPlacementPolicy{};
			}
			}

			throw JsonError(p_reader.file(), p_reader.path(), "unhandled opponent placement type");
		}

		[[nodiscard]] EncounterBoardPolicyDefinition parseBoard(
			JsonReader &p_reader,
			const Registry<HandcraftedBattleBoardDefinition> &p_boards)
		{
			const BoardPolicyKind kind = p_reader.requireEnum<BoardPolicyKind>("type", boardPolicyKindNames());

			switch (kind)
			{
			case BoardPolicyKind::LiveWorld:
			{
				// No "definition" key here, and no playerApproach: the geometry is the world the
				// player walked into, and the approach is the movement that triggered the battle.
				p_reader.forbidUnknown({"type", "size", "deploymentDepth", "opponentPlacement"});

				LiveWorldBoardPolicyDefinition policy;
				policy.size = p_reader.require<std::array<int, 2>>("size");
				for (const int side : policy.size)
				{
					if (side < MinimumBattleBoardSide || side > MaximumBattleBoardSide || side % 2 == 0)
					{
						throw JsonError(
							p_reader.file(),
							p_reader.pathFor("size"),
							"board sides are odd integers in [" + std::to_string(MinimumBattleBoardSide) + ", " +
								std::to_string(MaximumBattleBoardSide) + "], got " + std::to_string(side));
					}
				}

				const int narrowest = std::min(policy.size[0], policy.size[1]);
				policy.deploymentDepth =
					static_cast<int>(requireIntegerInRange(p_reader, "deploymentDepth", 1, narrowest));
				// The triggering movement supplies the approach, so either axis may end up being the
				// deployment one: both strips have to fit inside the narrower of the two.
				if (policy.deploymentDepth > narrowest - policy.deploymentDepth)
				{
					throw JsonError(
						p_reader.file(),
						p_reader.pathFor("deploymentDepth"),
						"both deployment strips are " + std::to_string(policy.deploymentDepth) +
							" rows deep and the board is only " + std::to_string(narrowest) +
							" rows across its narrower axis");
				}

				JsonReader placement = p_reader.child("opponentPlacement");
				policy.opponentPlacement = parsePlacement(placement);
				return policy;
			}
			case BoardPolicyKind::Handcrafted:
			{
				// Size, depth and approach all come from the definition; repeating them here would
				// be a second source of truth that a later edit could contradict.
				p_reader.forbidUnknown({"type", "definition", "opponentPlacement"});

				HandcraftedBoardPolicyDefinition policy;
				policy.definitionId = p_reader.require<std::string>("definition");
				requireContentId(
					policy.definitionId,
					p_reader.file(),
					p_reader.pathFor("definition"),
					"battle board id");
				if (!p_boards.contains(policy.definitionId))
				{
					throw JsonError(
						p_reader.file(),
						p_reader.pathFor("definition"),
						"unknown battle board '" + policy.definitionId + "'");
				}

				JsonReader placement = p_reader.child("opponentPlacement");
				policy.opponentPlacement = parsePlacement(placement);
				return policy;
			}
			}

			throw JsonError(p_reader.file(), p_reader.path(), "unhandled board policy type");
		}

		[[nodiscard]] EncounterSpawnDefinition parseSpawn(JsonReader &p_reader)
		{
			p_reader.forbidUnknown({"id", "species", "ai", "completedFeatNodes"});

			EncounterSpawnDefinition result;
			result.id = p_reader.require<std::string>("id");
			requireContentId(result.id, p_reader.file(), p_reader.pathFor("id"), "spawn id");
			result.speciesId = p_reader.require<std::string>("species");
			requireContentId(result.speciesId, p_reader.file(), p_reader.pathFor("species"), "species id");
			result.aiBehaviourId = p_reader.require<std::string>("ai");
			requireContentId(result.aiBehaviourId, p_reader.file(), p_reader.pathFor("ai"), "AI behaviour id");
			result.source = DefinitionSource{p_reader.file(), p_reader.path()};

			result.completedFeatNodeIds = p_reader.require<std::vector<std::string>>("completedFeatNodes");
			std::set<std::string> seen;
			for (const std::string &nodeId : result.completedFeatNodeIds)
			{
				requireContentId(nodeId, p_reader.file(), p_reader.pathFor("completedFeatNodes"), "feat node id");
				if (!seen.insert(nodeId).second)
				{
					throw JsonError(
						p_reader.file(),
						p_reader.pathFor("completedFeatNodes"),
						"duplicate completed node '" + nodeId + "'");
				}
			}
			return result;
		}

		[[nodiscard]] EncounterTeamDefinition parseTeam(JsonReader &p_reader)
		{
			p_reader.forbidUnknown({"id", "displayNameKey", "weight", "members"});

			EncounterTeamDefinition result;
			result.id = p_reader.require<std::string>("id");
			requireContentId(result.id, p_reader.file(), p_reader.pathFor("id"), "encounter team id");
			result.displayNameKey = requireDisplayNameKey(p_reader);
			result.weight = static_cast<std::uint64_t>(requireIntegerInRange(
				p_reader,
				"weight",
				static_cast<std::int64_t>(MinimumTeamWeight),
				static_cast<std::int64_t>(MaximumTeamWeight)));

			std::vector<JsonReader> members = p_reader.childArray("members");
			if (members.empty() || members.size() > MaximumTeamMembers)
			{
				throw JsonError(
					p_reader.file(),
					p_reader.pathFor("members"),
					"a team fields 1 to " + std::to_string(MaximumTeamMembers) + " creatures, got " +
						std::to_string(members.size()));
			}

			std::set<std::string> memberIds;
			result.members.reserve(members.size());
			for (JsonReader &member : members)
			{
				EncounterSpawnDefinition spawn = parseSpawn(member);
				if (!memberIds.insert(spawn.id).second)
				{
					throw JsonError(member.file(), member.path(), "duplicate member id '" + spawn.id + "'");
				}
				result.members.push_back(std::move(spawn));
			}
			return result;
		}

		[[nodiscard]] std::vector<EncounterTierDefinition> parseTiers(const JsonReader &p_reader)
		{
			std::vector<JsonReader> entries = p_reader.childArray("tiers");
			if (entries.empty())
			{
				throw JsonError(p_reader.file(), p_reader.pathFor("tiers"), "an encounter authors at least tier 0");
			}

			std::vector<EncounterTierDefinition> result;
			result.reserve(entries.size());
			for (JsonReader &entry : entries)
			{
				entry.forbidUnknown({"tier", "teams"});

				EncounterTierDefinition tier;
				tier.tier = static_cast<std::uint32_t>(
					requireIntegerInRange(entry, "tier", 0, static_cast<std::int64_t>(MaximumEncounterTier)));

				std::vector<JsonReader> teams = entry.childArray("teams");
				if (teams.empty())
				{
					throw JsonError(entry.file(), entry.pathFor("teams"), "a tier fields at least one team");
				}

				std::set<std::string> teamIds;
				std::uint64_t weightSum = 0;
				tier.teams.reserve(teams.size());
				for (JsonReader &team : teams)
				{
					EncounterTeamDefinition parsed = parseTeam(team);
					if (!teamIds.insert(parsed.id).second)
					{
						throw JsonError(team.file(), team.path(), "duplicate team id '" + parsed.id + "'");
					}
					// Checked in uint64: the weighted draw sums these, and a wrapped total would
					// silently make some teams unreachable.
					if (weightSum > std::numeric_limits<std::uint64_t>::max() - parsed.weight)
					{
						throw JsonError(team.file(), team.path(), "the tier's team weights overflow a 64-bit sum");
					}
					weightSum += parsed.weight;
					tier.teams.push_back(std::move(parsed));
				}

				// Strictly increasing, starting at 0: selection walks down to the greatest authored
				// tier at or below the request, and a sparse table with no tier 0 would have nothing
				// to fall back to.
				if (result.empty())
				{
					if (tier.tier != 0)
					{
						throw JsonError(entry.file(), entry.pathFor("tier"), "the first authored tier is tier 0");
					}
				}
				else if (tier.tier <= result.back().tier)
				{
					throw JsonError(
						entry.file(),
						entry.pathFor("tier"),
						"tiers are authored in strictly increasing order; " + std::to_string(tier.tier) +
							" follows " + std::to_string(result.back().tier));
				}
				result.push_back(std::move(tier));
			}
			return result;
		}

		void requireKindContract(const JsonReader &p_reader, const EncounterDefinition &p_encounter)
		{
			const std::string kind(toString(p_encounter.kind));
			const bool wild = p_encounter.kind == EncounterKind::Wild;

			if (p_encounter.allowsTaming && !wild)
			{
				throw JsonError(
					p_reader.file(),
					p_reader.pathFor("allowsTaming"),
					"only a wild encounter may be tamed; '" + kind + "' may not");
			}

			// Repeatability stays explicit in the file so that reading it needs no table, but it is
			// locked to the kind: a trainer, gym or special encounter clears permanently, and a wild
			// or debug encounter is always available.
			const bool mustRepeat = wild || p_encounter.kind == EncounterKind::Debug;
			if (p_encounter.repeatable != mustRepeat)
			{
				throw JsonError(
					p_reader.file(),
					p_reader.pathFor("repeatable"),
					"a '" + kind + "' encounter is " + (mustRepeat ? "repeatable" : "cleared once") +
						", so repeatable must be " + (mustRepeat ? "true" : "false"));
			}

			const bool handcrafted = std::holds_alternative<HandcraftedBoardPolicyDefinition>(p_encounter.board);
			const bool requiresHandcrafted =
				p_encounter.kind == EncounterKind::Gym || p_encounter.kind == EncounterKind::Special;
			const bool requiresLiveWorld = wild || p_encounter.kind == EncounterKind::Trainer;

			if (requiresHandcrafted && !handcrafted)
			{
				throw JsonError(
					p_reader.file(),
					p_reader.pathFor("board"),
					"a '" + kind + "' encounter fights in a handcrafted arena, not in the live world");
			}
			if (requiresLiveWorld && handcrafted)
			{
				throw JsonError(
					p_reader.file(),
					p_reader.pathFor("board"),
					"a '" + kind + "' encounter overlays the live world, so it takes no handcrafted board");
			}
			// A debug encounter may use either alternative, and obeys that alternative's whole schema.
		}

		// The enemy zone the placement has to fit inside, whichever board alternative was authored.
		// A live-world approach is only known when the player triggers the battle, so the definition
		// is checked against the narrower axis - the one that constrains it whichever way it lands.
		struct EnemyZone
		{
			int rowWidth = 0;
			int depth = 0;

			[[nodiscard]] std::size_t capacity() const noexcept
			{
				return static_cast<std::size_t>(rowWidth) * static_cast<std::size_t>(depth);
			}
		};

		[[nodiscard]] EnemyZone enemyZoneOf(
			const EncounterBoardPolicyDefinition &p_board,
			const Registry<HandcraftedBattleBoardDefinition> &p_boards)
		{
			if (const auto *live = std::get_if<LiveWorldBoardPolicyDefinition>(&p_board))
			{
				return EnemyZone{
					.rowWidth = std::min(live->size[0], live->size[1]),
					.depth = live->deploymentDepth};
			}

			const HandcraftedBattleBoardDefinition &board =
				p_boards.get(std::get<HandcraftedBoardPolicyDefinition>(p_board).definitionId);
			return EnemyZone{.rowWidth = deploymentRowWidth(board), .depth = board.deploymentDepth};
		}

		void validatePlacement(
			const JsonReader &p_reader,
			const EncounterDefinition &p_encounter,
			const Registry<HandcraftedBattleBoardDefinition> &p_boards)
		{
			const OpponentPlacementPolicy &placement = std::visit(
				[](const auto &p_policy) -> const OpponentPlacementPolicy & { return p_policy.opponentPlacement; },
				p_encounter.board);

			const EnemyZone zone = enemyZoneOf(p_encounter.board, p_boards);
			const std::array<int, 2> size = effectiveBoardSize(p_encounter.board, p_boards);
			const std::size_t largestTeam = largestTeamSize(p_encounter);
			const std::string path = p_reader.pathFor("board") + ".opponentPlacement";

			const auto fail = [&p_reader, &path](const std::string &p_message) {
				throw JsonError(p_reader.file(), path, p_message);
			};

			if (const auto *byLine = std::get_if<ByLineOpponentPlacementPolicy>(&placement))
			{
				if (byLine->rowsFromEnemyEdge >= zone.depth)
				{
					fail(
						"rowsFromEnemyEdge is in [0, " + std::to_string(zone.depth - 1) +
						"]; the enemy zone is only " + std::to_string(zone.depth) + " rows deep");
				}
				// Enumeration starts at that row and walks inward, never back toward the edge, so
				// the rows it skipped are not available to the team.
				const std::size_t reachable =
					static_cast<std::size_t>(zone.depth - byLine->rowsFromEnemyEdge) *
					static_cast<std::size_t>(zone.rowWidth);
				if (reachable < largestTeam)
				{
					fail(
						"the by-line placement exposes " + std::to_string(reachable) + " cells, and the largest team "
						"fields " + std::to_string(largestTeam) + " creatures");
				}
				return;
			}

			if (const auto *fixed = std::get_if<FixedOpponentPlacementPolicy>(&placement))
			{
				std::set<std::array<int, 2>> unique;
				for (const std::array<int, 2> &cell : fixed->cells)
				{
					if (cell[0] < 0 || cell[0] >= size[0] || cell[1] < 0 || cell[1] >= size[1])
					{
						fail(
							"cell [" + std::to_string(cell[0]) + ", " + std::to_string(cell[1]) +
							"] is outside the board's [0, " + std::to_string(size[0]) + ") x [0, " +
							std::to_string(size[1]) + ") extent");
					}
					if (!unique.insert(cell).second)
					{
						fail(
							"cell [" + std::to_string(cell[0]) + ", " + std::to_string(cell[1]) + "] is authored twice");
					}
				}
				if (fixed->cells.size() < largestTeam)
				{
					fail(
						"the fixed placement authors " + std::to_string(fixed->cells.size()) +
						" cells, and the largest team fields " + std::to_string(largestTeam) + " creatures");
				}

				// A handcrafted board knows its approach, so its enemy strip is knowable now. A
				// live-world one does not: its cells are checked against the concrete strip when the
				// triggering movement supplies the approach, and an incompatible one fails entry.
				if (const auto *handcrafted = std::get_if<HandcraftedBoardPolicyDefinition>(&p_encounter.board))
				{
					const HandcraftedBattleBoardDefinition &board = p_boards.get(handcrafted->definitionId);
					const BoardDeploymentStrip strip = enemyDeploymentStrip(board);
					for (const std::array<int, 2> &cell : fixed->cells)
					{
						if (!strip.contains(cell[0], cell[1]))
						{
							fail(
								"cell [" + std::to_string(cell[0]) + ", " + std::to_string(cell[1]) +
								"] is outside the enemy deployment strip of board '" + board.id + "'");
						}
					}
				}
				return;
			}

			if (zone.capacity() < largestTeam)
			{
				fail(
					"the seeded-random placement draws from " + std::to_string(zone.capacity()) +
					" enemy cells, and the largest team fields " + std::to_string(largestTeam) + " creatures");
			}
		}
	}

	const std::map<std::string, OpponentLineOrder> &opponentLineOrderNames()
	{
		static const std::map<std::string, OpponentLineOrder> names{
			{"centerOut", OpponentLineOrder::CenterOut},
			{"leftToRight", OpponentLineOrder::LeftToRight},
			{"rightToLeft", OpponentLineOrder::RightToLeft}};
		return names;
	}

	std::array<int, 2> effectiveBoardSize(
		const EncounterBoardPolicyDefinition &p_board,
		const Registry<HandcraftedBattleBoardDefinition> &p_boards)
	{
		if (const auto *live = std::get_if<LiveWorldBoardPolicyDefinition>(&p_board))
		{
			return live->size;
		}

		const HandcraftedBattleBoardDefinition &board =
			p_boards.get(std::get<HandcraftedBoardPolicyDefinition>(p_board).definitionId);
		return std::array<int, 2>{board.size.x, board.size.z};
	}

	int effectiveDeploymentDepth(
		const EncounterBoardPolicyDefinition &p_board,
		const Registry<HandcraftedBattleBoardDefinition> &p_boards)
	{
		if (const auto *live = std::get_if<LiveWorldBoardPolicyDefinition>(&p_board))
		{
			return live->deploymentDepth;
		}
		return p_boards.get(std::get<HandcraftedBoardPolicyDefinition>(p_board).definitionId).deploymentDepth;
	}

	std::size_t largestTeamSize(const EncounterDefinition &p_encounter) noexcept
	{
		std::size_t largest = 0;
		for (const EncounterTierDefinition &tier : p_encounter.tiers)
		{
			for (const EncounterTeamDefinition &team : tier.teams)
			{
				largest = std::max(largest, team.members.size());
			}
		}
		return largest;
	}

	EncounterDefinition parseEncounterDefinition(
		JsonReader &p_reader,
		const Registry<HandcraftedBattleBoardDefinition> &p_boards)
	{
		p_reader.forbidUnknown(
			{"version",
			 "displayNameKey",
			 "kind",
			 "allowsTaming",
			 "repeatable",
			 "triggerChancePermille",
			 "board",
			 "tiers"});
		requireVersion(p_reader, EncounterSchemaVersion);

		EncounterDefinition result;
		result.displayNameKey = requireDisplayNameKey(p_reader);
		result.kind = p_reader.requireEnum<EncounterKind>("kind", encounterKindNames());
		result.allowsTaming = p_reader.require<bool>("allowsTaming");
		result.repeatable = p_reader.require<bool>("repeatable");
		result.triggerChancePermille = static_cast<std::uint32_t>(requireIntegerInRange(
			p_reader,
			"triggerChancePermille",
			0,
			static_cast<std::int64_t>(MaximumTriggerChancePermille)));
		result.source = DefinitionSource{p_reader.file(), p_reader.path()};

		JsonReader board = p_reader.child("board");
		result.board = parseBoard(board, p_boards);
		result.tiers = parseTiers(p_reader);

		requireKindContract(p_reader, result);
		validatePlacement(p_reader, result, p_boards);
		return result;
	}
}
