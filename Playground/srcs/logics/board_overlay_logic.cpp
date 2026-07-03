#include "logics/board_overlay_logic.hpp"

#include "structures/graphics/rendering/command/spk_camera_update_render_command.hpp"
#include "structures/graphics/rendering/command/spk_directional_light_update_render_command.hpp"

namespace pg
{
	BoardOverlayLogic::BoardOverlayLogic(const spk::Texture &p_maskTexture) :
		_maskTexture(p_maskTexture)
	{
	}

	BoardOverlayLogic::CachedDraw &BoardOverlayLogic::_cachedDraw(const BoardOverlayView &p_view)
	{
		CachedDraw &cached = _cache[&p_view];
		if (cached.modelUBOs[0] == nullptr)
		{
			for (std::size_t index = 0; index < OverlayCategoryCount; ++index)
			{
				const spk::Matrix4x4 model = spk::Matrix4x4::translation(0.0f, 0.001f * static_cast<float>(index), 0.0f);
				cached.modelUBOs[index] =
					spk::DrawTextureMesh3DRenderCommand::makeModelUBO(model, spk::Color{1.0f, 1.0f, 1.0f, 0.7f});
			}
		}
		return cached;
	}

	void BoardOverlayLogic::_parseComponentForUpdate(const spk::UpdateTick &p_tick, BoardOverlayView &p_view)
	{
		(void)p_tick;
		p_view.rebuildIfNeeded();
	}

	void BoardOverlayLogic::_onRenderStarted(std::size_t p_componentCount)
	{
		(void)p_componentCount;
		_commands.clear();
		_camera = spk::Camera3D::mainCamera();
		if (_camera != nullptr)
		{
			_cameraMatrix = _camera->viewProjectionMatrix();
		}
		if (_maskSampler == nullptr)
		{
			_maskSampler = spk::DrawTextureMesh3DRenderCommand::makeSampler(_maskTexture);
		}
	}

	void BoardOverlayLogic::_parseComponentForRender(BoardOverlayView &p_view)
	{
		if (_camera == nullptr || p_view.bound() == false)
		{
			return;
		}
		CachedDraw &cached = _cachedDraw(p_view);
		for (std::size_t index = 0; index < OverlayCategoryCount; ++index)
		{
			const std::shared_ptr<const spk::TextureMesh3D> &mesh = p_view.mesh(static_cast<OverlayCategory>(index));
			if (mesh == nullptr || mesh->indexes().empty())
			{
				continue;
			}
			_commands.push_back(std::make_unique<spk::DrawTextureMesh3DRenderCommand>(mesh, cached.modelUBOs[index], _maskSampler, true));
		}
	}

	void BoardOverlayLogic::_executeRender(spk::RenderUnitBuilder &p_builder)
	{
		if (_camera == nullptr || _commands.empty())
		{
			return;
		}
		p_builder.emplace<spk::CameraUpdateRenderCommand>(CameraBinding, _cameraMatrix);
		p_builder.emplace<spk::DirectionalLightUpdateRenderCommand>(
			DirectionalLightBinding,
			spk::DirectionalLight{
				.direction = spk::Vector3(1.0f, -2.0f, 0.5f).normalized(),
				.color = spk::Color(1.0f, 0.95f, 0.85f),
				.ambient = AmbientLight});
		for (std::unique_ptr<spk::DrawTextureMesh3DRenderCommand> &command : _commands)
		{
			p_builder.add(std::move(command));
		}
		_commands.clear();
	}
}
