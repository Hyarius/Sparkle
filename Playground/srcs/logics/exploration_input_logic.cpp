#include "logics/exploration_input_logic.hpp"

#include "battle/presentation/battle_presentation_cell_source.hpp"
#include "rendering/mouse_picker.hpp"
#include "rendering/walk_surface_mask_mesh_builder.hpp"
#include "world/voxel_world.hpp"
#include "world/world_navigation.hpp"

#include <memory>
#include <utility>

namespace pg
{
	ExplorationInputLogic::ExplorationInputLogic(
		GameContext &p_context,
		VoxelWorld &p_world,
		WorldNavigation &p_navigation,
		spk::Camera3D &p_camera,
		spk::TextureMeshRenderer3D &p_hoverRenderer,
		ViewportSize p_viewportSize,
		spk::AtlasCell p_hoveredMask,
		spk::AtlasCell p_invalidMask) :
		_context(p_context),
		_world(p_world),
		_navigation(p_navigation),
		_camera(p_camera),
		_hoverRenderer(p_hoverRenderer),
		_viewportSize(std::move(p_viewportSize)),
		_hoveredMask(p_hoveredMask),
		_invalidMask(p_invalidMask),
		_presentationCells(_world),
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
		const spk::Ray3D ray = _camera.rayFromViewport(viewport, spk::Vector2(p_mouse));
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
		const spk::AtlasCell mask = _invalidSeconds > 0 ? _invalidMask : _hoveredMask;
		_hoverRenderer.setMesh(std::make_shared<spk::TextureMesh3D>(WalkSurfaceMaskMeshBuilder::buildOne(_presentationCells, *_hovered, mask)));
	}

	const std::optional<spk::Vector3Int> &ExplorationInputLogic::hoveredCell() const noexcept
	{
		return _hovered;
	}

	void ExplorationInputLogic::_parseComponentForUpdate(const spk::UpdateContext &p_tick, Actor &p_actor)
	{
		if (!_context.isExplorationActive() || !p_actor.player || _invalidSeconds <= 0)
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
		if (!_context.isExplorationActive() || !p_actor.player)
		{
			return;
		}
		_pick(p_event->position);
	}

	void ExplorationInputLogic::_parseComponentForMouseButtonPressedEvent(
		spk::MouseButtonPressedEvent &p_event,
		Actor &p_actor)
	{
		if (!_context.isExplorationActive() || !p_actor.player || p_event->button != spk::Mouse::Left)
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
