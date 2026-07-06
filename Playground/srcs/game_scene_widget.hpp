#pragma once

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <optional>
#include <string>

#include "components/actor.hpp"
#include "core/game_context.hpp"
#include "structures/game_engine/spk_camera_3d.hpp"
#include "structures/game_engine/spk_entity_3d.hpp"
#include "structures/graphics/geometry/spk_texture_mesh_3d.hpp"
#include "structures/graphics/texture/spk_sprite_sheet.hpp"
#include "structures/widget/spk_debug_overlay.hpp"
#include "structures/widget/spk_game_engine_widget.hpp"
#include "world/chunk_coordinates.hpp"

namespace pg
{
	class Registries;
	class ExplorationInputLogic;
	class ActorPathLogic;
	class ProceduralWorld;
	class ProceduralChunkProvider;
	class WorldStreamer;

	class GameSceneWidget : public spk::GameEngineWidget
	{
	private:
		GameContext &_context;
		std::uint64_t _worldSeed = 1;
		std::unique_ptr<ProceduralWorld> _proceduralWorld;
		std::unique_ptr<ProceduralChunkProvider> _proceduralProvider;
		std::unique_ptr<WorldStreamer> _worldStreamer;
		std::optional<ChunkCoordinates> _streamingFocus;

		spk::SpriteSheet _texture;
		spk::SpriteSheet _maskTexture;
		std::shared_ptr<spk::TextureMesh3D> _playerMesh;
		spk::Entity3D _playerEntity;
		spk::Entity3D _hoverEntity;
		Actor *_player = nullptr;
		ExplorationInputLogic *_inputLogic = nullptr;
		ActorPathLogic *_pathLogic = nullptr;

		spk::Entity3D _cameraEntity;
		spk::Camera3D *_camera = nullptr;
		spk::DebugOverlay _overlay;

		mutable std::atomic<long long> _renderDurationNs{0};
		std::atomic<long long> _updateDurationNs{0};
		mutable std::atomic<std::size_t> _meshCount{0};
		mutable std::atomic<std::size_t> _triangleCount{0};

		void _buildScene(const Registries &p_registries);
		void _configureOverlay();
		void _refreshOverlay(const spk::UpdateTick &p_tick);

	protected:
		void _onGeometryChange() override;
		void _onUpdate(const spk::UpdateTick &p_tick) override;
		void _onKeyPressedEvent(spk::KeyPressedEvent &p_event) override;
		[[nodiscard]] spk::RenderUnit _buildRenderUnit() const override;

	public:
		GameSceneWidget(
			const std::string &p_name,
			spk::Widget *p_parent,
			GameContext &p_context,
			const Registries &p_registries,
			std::uint64_t p_worldSeed = 1);
		~GameSceneWidget() override;
	};
}
