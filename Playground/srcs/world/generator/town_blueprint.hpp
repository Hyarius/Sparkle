#pragma once

#include "core/json.hpp"
#include "core/registry.hpp"

#include <map>
#include <optional>
#include <string>
#include <vector>

namespace pg
{
	enum class TownBlueprintKind { City, Gym, Port };
	enum class TownBlueprintRole { CreatureCenter, Shop, Gym, Port, Home };

	struct LocalCell { int row = 0; int col = 0; friend bool operator==(const LocalCell &, const LocalCell &) = default; };
	struct LocalRect { LocalCell min{}; LocalCell max{}; };
	struct LocalEndpoint { std::string id; LocalCell cell{}; };
	struct TownLotTemplate { std::string id; TownBlueprintRole role{}; LocalRect reservation{}; std::string approach; bool required = true; };
	struct TownPathTemplate { std::string id; std::string from; std::string to; std::vector<LocalCell> surface; };
	struct TownDecorationZone { std::string id; std::vector<LocalCell> cells; };
	struct TownWaterfront { std::vector<LocalCell> dock; LocalCell boarding{}; };
	struct TownTerraceTemplate { std::string id; int levelOffset=0; std::vector<LocalCell> cells; };
	struct TownStairTemplate { std::string id; std::string lower; std::string upper; int expectedLevelDelta=1; LocalRect reservation{}; };
	struct TownBlueprint
	{
		std::string id;
		TownBlueprintKind kind{};
		LocalRect bounds{};
		std::vector<int> allowedQuarterTurns;
		std::vector<LocalEndpoint> endpoints;
		std::vector<TownLotTemplate> lots;
		std::vector<TownPathTemplate> paths;
		std::vector<TownDecorationZone> decorationZones;
		std::vector<TownTerraceTemplate> terraces;
		std::vector<TownStairTemplate> stairs;
		std::vector<LocalCell> boundary;
		std::optional<TownWaterfront> waterfront;
	};

	[[nodiscard]] LocalCell rotateTownCell(LocalCell p_cell, LocalRect p_bounds, int p_quarterTurns);
	[[nodiscard]] LocalRect rotateTownRect(LocalRect p_rect, LocalRect p_bounds, int p_quarterTurns);
	[[nodiscard]] std::string renderTownBlueprintAscii(const TownBlueprint &p_blueprint, int p_quarterTurns);
	[[nodiscard]] TownBlueprint parseTownBlueprint(JsonReader &p_reader);

	using TownBlueprintCatalog = Registry<TownBlueprint>;
}
