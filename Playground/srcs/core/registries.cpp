#include "core/registries.hpp"

#include "core/json.hpp"

namespace pg
{
	void Registries::loadAll(const std::filesystem::path &p_dataDirectory)
	{
		const std::filesystem::path gameRulesFile = p_dataDirectory / "config" / "game-rules.json";
		nlohmann::json json = JsonLoader::parseFile(gameRulesFile);
		JsonReader reader(json, gameRulesFile);
		GameRules loadedGameRules = parseGameRules(reader);

		_gameRules = std::move(loadedGameRules);
	}

	const GameRules &Registries::gameRules() const noexcept
	{
		return _gameRules;
	}
}
