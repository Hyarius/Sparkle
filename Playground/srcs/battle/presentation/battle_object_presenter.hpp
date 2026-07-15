#pragma once

#include "battle/battle_event.hpp"
#include "battle/battle_snapshot.hpp"
#include "battle/presentation/battle_object_view.hpp"
#include "structures/graphics/geometry/spk_texture_mesh_3d.hpp"

#include <map>
#include <memory>
#include <span>

namespace spk
{
	class GameEngine;
	class Texture;
	struct UpdateContext;
}

namespace pg
{
	struct BattlePresentationBoardBinding;

	class BattleObjectPresenter
	{
	private:
		spk::GameEngine *_engine = nullptr;
		const BattlePresentationBoardBinding *_board = nullptr;
		const spk::Texture *_texture = nullptr;
		std::shared_ptr<const spk::TextureMesh3D> _mesh;
		std::map<BattleObjectId, BattleObjectView> _views;
		BattleEventSequence _nextSequence{};

		void _createOrUpdate(const BattleObjectSnapshot &p_object);
		void _remove(BattleObjectId p_id) noexcept;
		void _reconcile(const BattleSnapshot &p_snapshot);

	public:
		void attach(
			spk::GameEngine &p_engine,
			const BattlePresentationBoardBinding &p_boardPresentation,
			const spk::Texture &p_texture,
			const BattleSnapshot &p_snapshot,
			BattleEventSequence p_nextSequence);
		void consume(std::span<const BattleEvent> p_events, const BattleSnapshot &p_afterBatch);
		void update(const spk::UpdateContext &p_tick);
		void fastForward();
		void detach() noexcept;
		[[nodiscard]] std::size_t viewCount() const noexcept;
	};
}
