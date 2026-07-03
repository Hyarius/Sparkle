#pragma once

#include "encounters/encounter_resolver.hpp"
#include "structures/design_pattern/spk_contract_provider.hpp"

#include <chrono>

namespace pg
{
	struct GameContext;

	class EncounterEmitter
	{
	private:
		GameContext &_context;
		EncounterResolver _resolver;
		spk::Vector2Int _boardSize;
		bool _enabled = false;
		std::chrono::steady_clock::time_point _frozenUntil{};
		spk::ContractProvider<spk::Vector3Int>::Contract _moveContract;

		void _onPlayerMoved(const spk::Vector3Int &p_standableCell);

	public:
		EncounterEmitter(GameContext &p_context, spk::Vector2Int p_boardSize);

		void setEnabled(bool p_enabled) noexcept;
		void suppressCell(const spk::Vector3Int &p_cell) noexcept;
		[[nodiscard]] bool enabled() const noexcept;
	};

	[[nodiscard]] std::string encounterSummary(const EncounterSpawn &p_spawn);
}
