#include "player/new_game_definition.hpp"

#include "core/content_id.hpp"
#include "core/definition_fields.hpp"
#include "core/registries.hpp"

#include <exception>
#include <set>
#include <stdexcept>

namespace pg
{
	namespace
	{
		[[nodiscard]] NewGameCreatureDefinition parseStarter(JsonReader &p_reader)
		{
			p_reader.forbidUnknown({"species", "completedFeatNodes"});

			NewGameCreatureDefinition result;
			result.speciesId = p_reader.require<std::string>("species");
			requireContentId(result.speciesId, p_reader.file(), p_reader.pathFor("species"), "species id");

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
	}

	NewGameDefinition parseNewGameDefinition(JsonReader &p_reader)
	{
		p_reader.forbidUnknown({"version", "starterTeam"});
		requireVersion(p_reader, NewGameSchemaVersion);

		NewGameDefinition result;
		result.source = DefinitionSource{p_reader.file(), p_reader.path()};

		std::vector<JsonReader> entries = p_reader.childArray("starterTeam");
		if (entries.empty() || entries.size() > PlayerRoster::TeamCapacity)
		{
			throw JsonError(
				p_reader.file(),
				p_reader.pathFor("starterTeam"),
				"a new game starts with 1 to " + std::to_string(PlayerRoster::TeamCapacity) + " creatures, got " +
					std::to_string(entries.size()));
		}

		result.starterTeam.reserve(entries.size());
		for (JsonReader &entry : entries)
		{
			result.starterTeam.push_back(parseStarter(entry));
		}
		return result;
	}

	PlayerData makeNewPlayerData(const NewGameDefinition &p_definition, const DerivationContext &p_context)
	{
		PlayerData result;

		for (std::size_t index = 0; index < p_definition.starterTeam.size(); ++index)
		{
			const NewGameCreatureDefinition &starter = p_definition.starterTeam[index];
			try
			{
				const PlayerRoster::Placement placement =
					createAndAddCreature(result, starter.speciesId, starter.completedFeatNodeIds, p_context);
				// The array is 1 to 6 entries long and the roster starts empty, so every starter
				// takes the team slot its authored position promised.
				if (!placement.inTeam || placement.index != index)
				{
					throw std::invalid_argument("starter " + std::to_string(index) + " did not take its own team slot");
				}
			} catch (const JsonError &)
			{
				throw;
			} catch (const std::exception &error)
			{
				throw JsonError(
					p_definition.source.file,
					p_definition.source.jsonPath + ".starterTeam[" + std::to_string(index) + "]",
					std::string("starter '") + starter.speciesId + "' is not a legal creature: " + error.what());
			}
		}
		return result;
	}

	PlayerData makeNewPlayerData(const NewGameDefinition &p_definition, const Registries &p_registries)
	{
		return makeNewPlayerData(p_definition, derivationContextOf(p_registries));
	}
}
