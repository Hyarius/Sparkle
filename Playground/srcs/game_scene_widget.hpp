#pragma once

#include <atomic>
#include <cstddef>
#include <string>

#include "structures/game_engine/spk_entity_2d.hpp"
#include "structures/graphics/geometry/spk_texture_mesh_2d.hpp"
#include "structures/graphics/texture/spk_sprite_sheet.hpp"
#include "structures/widget/spk_debug_overlay.hpp"
#include "structures/widget/spk_game_engine_widget.hpp"

namespace pg
{
	class GameSceneWidget : public spk::GameEngineWidget
	{
	private:
		spk::SpriteSheet _spriteSheet;
		spk::TextureMesh2D _spriteMesh;

		spk::Entity2D _player;
		spk::Entity2D _cameraEntity{&_player};
		spk::Entity2D _objectA;
		spk::Entity2D _objectB;

		spk::DebugOverlay _overlay;

		mutable std::atomic<long long> _renderDurationNs{0};
		std::atomic<long long> _updateDurationNs{0};
		mutable std::atomic<std::size_t> _spriteCount{0};
		mutable std::atomic<std::size_t> _polygonCount{0};

		void _buildScene();
		void _configureOverlay();
		void _refreshOverlay(const spk::UpdateTick &p_tick);

	protected:
		void _onGeometryChange() override;
		void _onUpdate(const spk::UpdateTick &p_tick) override;
		[[nodiscard]] spk::RenderUnit _buildRenderUnit() const override;

	public:
		GameSceneWidget(const std::string &p_name, spk::Widget *p_parent);
	};
}
