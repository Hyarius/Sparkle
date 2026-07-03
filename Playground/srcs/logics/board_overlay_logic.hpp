#pragma once

#include "components/board_overlay_view.hpp"
#include "structures/game_engine/spk_camera_3d.hpp"
#include "structures/game_engine/spk_component_logic.hpp"
#include "structures/graphics/rendering/command/spk_draw_texture_mesh_3d_render_command.hpp"
#include "structures/graphics/spk_sampler_object.hpp"
#include "structures/graphics/spk_texture.hpp"
#include "structures/graphics/spk_uniform_buffer_object.hpp"
#include "structures/math/spk_matrix.hpp"

#include <array>
#include <memory>
#include <unordered_map>
#include <vector>

namespace pg
{
	// Rebuilds overlay meshes when their state changes (update phase) and emits the translucent
	// mask draws after the terrain (render phase). Each category renders one stacked layer up
	// (0.001 Y) so overlapping categories don't z-fight — deployment lowest, invalid on top.
	class BoardOverlayLogic : public spk::ComponentLogic<BoardOverlayView>
	{
	private:
		static constexpr GLuint CameraBinding = 1;
		static constexpr GLuint DirectionalLightBinding = 3;
		static constexpr float AmbientLight = 0.35f;

		struct CachedDraw
		{
			std::array<std::shared_ptr<spk::UniformBufferObject>, OverlayCategoryCount> modelUBOs{};
		};

		const spk::Texture &_maskTexture;
		std::shared_ptr<spk::SamplerObject> _maskSampler;
		spk::Camera3D *_camera = nullptr;
		spk::Matrix4x4 _cameraMatrix;
		std::unordered_map<const BoardOverlayView *, CachedDraw> _cache;
		std::vector<std::unique_ptr<spk::DrawTextureMesh3DRenderCommand>> _commands;

		[[nodiscard]] CachedDraw &_cachedDraw(const BoardOverlayView &p_view);

	public:
		explicit BoardOverlayLogic(const spk::Texture &p_maskTexture);

	protected:
		void _parseComponentForUpdate(const spk::UpdateTick &p_tick, BoardOverlayView &p_view) override;

		void _onRenderStarted(std::size_t p_componentCount) override;
		void _parseComponentForRender(BoardOverlayView &p_view) override;
		void _executeRender(spk::RenderUnitBuilder &p_builder) override;
	};
}
