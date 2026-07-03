#pragma once

#include "battle/battle_event.hpp"
#include "components/battle_unit_view.hpp"
#include "structures/design_pattern/spk_contract_provider.hpp"
#include "structures/game_engine/spk_component_logic.hpp"
#include "structures/game_engine/spk_entity_3d.hpp"
#include "structures/graphics/geometry/spk_texture_mesh_3d.hpp"

#include <map>
#include <memory>

namespace spk
{
	class GameEngine;
	class Texture;
}

namespace pg
{
	class BattleContext;
	class BattleUnit;

	// Owns the transient Entity3D registry used to render a battle. begin/synchronize/end are
	// explicit so placement and teardown are deterministic and testable without a window.
	class BattleUnitViewLogic : public spk::ComponentLogic<BattleUnitView>
	{
	private:
		struct Entry
		{
			std::unique_ptr<spk::Entity3D> entity;
			BattleUnitView *view = nullptr;
		};

		spk::GameEngine &_engine;
		const spk::Texture *_texture = nullptr;
		std::shared_ptr<const spk::TextureMesh3D> _mesh;
		BattleContext *_context = nullptr;
		std::map<BattleUnit *, Entry> _entries;
		spk::ContractProvider<const BattleEvent *>::Contract _eventContract;
		spk::ContractProvider<BattleUnit *>::Contract _placementContract;

		void _onBattleEvent(const BattleEvent *p_event);
		void _spawn(BattleUnit &p_unit, const spk::Vector3Int &p_cell);
		void _place(BattleUnitView &p_view, const spk::Vector3 &p_localPosition);
		void _beginMove(BattleUnit &p_unit, const spk::Vector3Int &p_destination);
		void _beginDefeat(BattleUnit &p_unit);

	public:
		static constexpr float MoveSpeed = 4.0f;
		static constexpr float SinkDuration = 0.45f;

		explicit BattleUnitViewLogic(spk::GameEngine &p_engine, const spk::Texture *p_texture = nullptr);
		~BattleUnitViewLogic() override;

		void begin(BattleContext &p_context);
		void synchronize();
		void advance(float p_seconds);
		void end();

		[[nodiscard]] bool viewBusy() const noexcept;
		[[nodiscard]] std::size_t registeredViewCount() const noexcept;
		[[nodiscard]] std::size_t activeViewCount() const noexcept;
		[[nodiscard]] const BattleUnitView *find(const BattleUnit &p_unit) const noexcept;

	protected:
		void _executeUpdate(const spk::UpdateTick &p_tick) override;
	};
}
