#pragma once

#include "battle/battle_event.hpp"
#include "battle/battle_snapshot.hpp"
#include "battle/presentation/battle_unit_view.hpp"
#include "creatures/creature_species_definition.hpp"
#include "structures/graphics/geometry/spk_texture_mesh_3d.hpp"

#include <map>
#include <span>

namespace spk
{
	class GameEngine;
	class Texture;
	struct UpdateContext;
}

namespace pg
{
	class Registries;
	struct BattlePresentationBoardBinding;

	// Owns only presentation entities.  It reads copied snapshots and committed events; neither
	// animation nor reconciliation has a path back into BattleSession.
	class BattleUnitPresenter
	{
	private:
		spk::GameEngine *_engine = nullptr;
		const BattlePresentationBoardBinding *_board = nullptr;
		const Registries *_registries = nullptr;
		const spk::Texture *_texture = nullptr;
		std::shared_ptr<const spk::TextureMesh3D> _cubeMesh;
		std::map<BattleUnitId, BattleUnitView> _views;
		BattleSnapshot _snapshot;
		BattleEventSequence _nextSequence{};

		[[nodiscard]] PlaceholderVisual _visualFor(const BattleUnitSnapshot &p_unit) const;
		void _createOrUpdate(const BattleUnitSnapshot &p_unit, bool p_snap);
		void _remove(BattleUnitId p_id) noexcept;
		void _reconcile(const BattleSnapshot &p_snapshot, bool p_snap);
		void _startTrack(BattleUnitId p_unit, UnitCosmeticTrack p_track);

	public:
		void attach(
			spk::GameEngine &p_engine,
			const BattlePresentationBoardBinding &p_boardPresentation,
			const Registries &p_registries,
			const spk::Texture &p_texture,
			const BattleSnapshot &p_snapshot,
			BattleEventSequence p_nextSequence);
		void consume(std::span<const BattleEvent> p_events, const BattleSnapshot &p_afterBatch);
		void update(const spk::UpdateContext &p_tick);
		void fastForward();
		void detach() noexcept;

		[[nodiscard]] bool attached() const noexcept;
		[[nodiscard]] std::size_t viewCount() const noexcept;
		[[nodiscard]] BattleEventSequence nextSequence() const noexcept;
	};
}
