#include "logics/exploration_input_logic.hpp"

#include "board/cell_source.hpp"
#include "rendering/mouse_picker.hpp"
#include "structures/graphics/geometry/spk_texture_mesh_3d.hpp"
#include "world/voxel_world.hpp"
#include "world/world_navigation.hpp"

#include <memory>
#include <utility>

namespace
{
	// The overlay masks index into the 4x4 mask atlas (resources/textures/mask.png).
	constexpr float MaskAtlasColumns = 4.0f;
	constexpr float MaskAtlasRows = 4.0f;

	[[nodiscard]] spk::Vector2 maskUV(const pg::AtlasCell &p_cell, float p_u, float p_v)
	{
		return {(static_cast<float>(p_cell.column) + p_u) / MaskAtlasColumns, (static_cast<float>(p_cell.row) + p_v) / MaskAtlasRows};
	}

	// A flat highlight quad sitting on the walk surface of the hovered cell, textured with a
	// mask atlas cell. This replaces the old shape-conforming mask mesh (which depended on the
	// legacy voxel shapes) now that chunk baking lives in the spk voxel library.
	[[nodiscard]] spk::TextureMesh3D buildHoverQuad(const spk::Vector3Int &p_cell, float p_height, const pg::AtlasCell &p_mask)
	{
		const float x0 = static_cast<float>(p_cell.x);
		const float x1 = x0 + 1.0f;
		const float z0 = static_cast<float>(p_cell.z);
		const float z1 = z0 + 1.0f;
		const float y = p_height + 0.02f;
		const spk::Vector3 normal{0.0f, 1.0f, 0.0f};

		spk::TextureMesh3D::Builder builder;
		builder.addShape(
			spk::TextureVertex3D{{x0, y, z1}, normal, maskUV(p_mask, 0.0f, 0.0f)},
			spk::TextureVertex3D{{x1, y, z1}, normal, maskUV(p_mask, 1.0f, 0.0f)},
			spk::TextureVertex3D{{x1, y, z0}, normal, maskUV(p_mask, 1.0f, 1.0f)},
			spk::TextureVertex3D{{x0, y, z0}, normal, maskUV(p_mask, 0.0f, 1.0f)});
		return builder.bake();
	}
}

namespace pg
{
	ExplorationInputLogic::ExplorationInputLogic(
		GameContext &p_context,
		VoxelWorld &p_world,
		WorldNavigation &p_navigation,
		spk::Camera3D &p_camera,
		spk::TextureMeshRenderer3D &p_hoverRenderer,
		ViewportSize p_viewportSize,
		AtlasCell p_hoveredMask,
		AtlasCell p_invalidMask) :
		_context(p_context),
		_world(p_world),
		_navigation(p_navigation),
		_camera(p_camera),
		_hoverRenderer(p_hoverRenderer),
		_viewportSize(std::move(p_viewportSize)),
		_hoveredMask(p_hoveredMask),
		_invalidMask(p_invalidMask),
		_invalidContract(_context.events.invalidTarget.subscribe([this](spk::Vector3Int p_cell) {
			_hovered = p_cell;
			_invalidSeconds = 0.3f;
			_rebuildHover();
		}))
	{
	}

	void ExplorationInputLogic::_pick(const spk::Vector2Int &p_mouse)
	{
		const spk::Vector2 viewport = _viewportSize();
		const WorldRay ray = MousePicker::screenToRay(_camera, viewport, spk::Vector2(p_mouse));
		_hovered = MousePicker::pickStandable(_world, _navigation, ray);
		_invalidSeconds = 0;
		_rebuildHover();
	}

	void ExplorationInputLogic::_rebuildHover()
	{
		// Swap in a brand-new mesh rather than mutating the current one: the render thread may
		// still be drawing the previous hover mesh, and it keeps that one alive via its own
		// shared_ptr until the frame that referenced it is retired.
		if (!_hovered.has_value())
		{
			_hoverRenderer.setMesh(std::make_shared<spk::TextureMesh3D>());
			return;
		}
		const AtlasCell mask = _invalidSeconds > 0 ? _invalidMask : _hoveredMask;
		const float height = walkHeightAtCenter(WorldCellSource(_world), *_hovered);
		_hoverRenderer.setMesh(std::make_shared<spk::TextureMesh3D>(buildHoverQuad(*_hovered, height, mask)));
	}

	const std::optional<spk::Vector3Int> &ExplorationInputLogic::hoveredCell() const noexcept
	{
		return _hovered;
	}

	void ExplorationInputLogic::_parseComponentForUpdate(const spk::UpdateTick &p_tick, Actor &p_actor)
	{
		if (!_context.world.explorationActive || !p_actor.player || _invalidSeconds <= 0)
		{
			return;
		}
		_invalidSeconds -= static_cast<float>(p_tick.deltaTime.seconds());
		if (_invalidSeconds <= 0)
		{
			_rebuildHover();
		}
	}

	void ExplorationInputLogic::_parseComponentForMouseMovedEvent(spk::MouseMovedEvent &p_event, Actor &p_actor)
	{
		if (!_context.world.explorationActive || !p_actor.player)
		{
			return;
		}
		_pick(p_event->position);
	}

	void ExplorationInputLogic::_parseComponentForMouseButtonPressedEvent(
		spk::MouseButtonPressedEvent &p_event,
		Actor &p_actor)
	{
		if (!_context.world.explorationActive || !p_actor.player || p_event->button != spk::Mouse::Left)
		{
			return;
		}
		_pick(p_event.device().position);
		if (_hovered.has_value())
		{
			_context.events.actorMoveRequested.trigger(&p_actor, *_hovered);
		}
	}
}
