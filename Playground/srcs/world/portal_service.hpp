#pragma once

#include "battle/battle_side.hpp"
#include "core/game_context.hpp"
#include "structures/design_pattern/spk_contract_provider.hpp"

#include <functional>
#include <string>

namespace pg
{
	class Actor;
	class Registries;
	struct MapPortal;

	class PortalService
	{
	public:
		using RelocateFunction = std::function<void(Actor &)>;
		using TransitionFunction = std::function<void()>;

	private:
		GameContext &_context;
		const Registries &_registries;
		Actor &_player;
		RelocateFunction _relocate;
		TransitionFunction _transitioned;
		bool _transitioning = false;
		bool _respawnPending = false;
		spk::ContractProvider<spk::Vector3Int>::Contract _moveContract;
		spk::ContractProvider<BattleContext *, BattleSide>::Contract _battleResolvedContract;
		spk::ContractProvider<>::Contract _battleEndContract;

		void _onPlayerMoved(const spk::Vector3Int &p_cell);
		void _refreshPrompt(const spk::Vector3Int &p_cell);
		[[nodiscard]] const MapPortal &_requirePortal(const std::string &p_mapId, const std::string &p_portalName) const;

	public:
		PortalService(
			GameContext &p_context,
			const Registries &p_registries,
			Actor &p_player,
			RelocateFunction p_relocate = {},
			TransitionFunction p_transitioned = {});

		void transitionToPortal(const std::string &p_mapId, const std::string &p_portalName);
		void transitionTo(const WorldPosition &p_position);
		void healParty();
		[[nodiscard]] bool atHealPoint(const spk::Vector3Int &p_cell) const;
	};
}
