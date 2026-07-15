#include "logics/exploration_input_logic.hpp"

#include "rendering/mouse_picker.hpp"
#include "structures/graphics/geometry/spk_texture_mesh_3d.hpp"
#include "structures/voxel/spk_voxel_shape.hpp"
#include "world/voxel_world.hpp"
#include "world/world_navigation.hpp"

#include <algorithm>
#include <cstddef>
#include <memory>
#include <utility>
#include <vector>

namespace
{
	// The overlay masks index into the 4x4 mask atlas (resources/textures/mask.png).
	constexpr float MaskAtlasColumns = 4.0f;
	constexpr float MaskAtlasRows = 4.0f;

	// Small vertical lift so the highlight floats just above the traced surface instead of
	// z-fighting the voxel face it sits on.
	constexpr float MaskLift = 0.02f;

	[[nodiscard]] spk::Vector2 maskUV(const spk::AtlasCell &p_cell, float p_u, float p_v)
	{
		return {(static_cast<float>(p_cell.column) + p_u) / MaskAtlasColumns, (static_cast<float>(p_cell.row) + p_v) / MaskAtlasRows};
	}

	// Emits every upward-facing polygon of an already-oriented voxel face, retextured with the
	// mask atlas cell. A polygon counts as "up" when its transformed normal has a positive Y
	// component, which picks out exactly the walk surface of each shape (a cube/slab top, the
	// slanted quad of a slope, the treads of a stair) while dropping walls and undersides.
	void appendWalkSurface(
		spk::TextureMesh3D::Builder &p_builder,
		const spk::VoxelShapeFace &p_face,
		const spk::Vector3 &p_offset,
		const spk::AtlasCell &p_mask)
	{
		for (const spk::VoxelShapePolygon &polygon : p_face.polygons())
		{
			if (polygon.size() < 3 || polygon.normal().y <= 0.0001f)
			{
				continue;
			}

			// The shape carries voxel-atlas UVs; normalise each polygon back to its own 0..1
			// square so the mask cell stretches across the whole surface whatever the shape.
			spk::Vector2 minimum = polygon[0].data;
			spk::Vector2 maximum = polygon[0].data;
			for (const spk::VoxelShapeVertex &vertex : polygon)
			{
				minimum.x = std::min(minimum.x, vertex.data.x);
				minimum.y = std::min(minimum.y, vertex.data.y);
				maximum.x = std::max(maximum.x, vertex.data.x);
				maximum.y = std::max(maximum.y, vertex.data.y);
			}
			const spk::Vector2 span = maximum - minimum;

			std::vector<spk::TextureVertex3D> vertices;
			vertices.reserve(polygon.size());
			for (const spk::VoxelShapeVertex &vertex : polygon)
			{
				const float localU = span.x > 0.0f ? (vertex.data.x - minimum.x) / span.x : 0.0f;
				const float localV = span.y > 0.0f ? (vertex.data.y - minimum.y) / span.y : 0.0f;
				vertices.push_back({vertex.position + p_offset, polygon.normal(), maskUV(p_mask, localU, localV)});
			}
			p_builder.addShape(vertices);
		}
	}

	// A highlight tracing the walk surface of the hovered voxel, lifted slightly so it reads as
	// an overlay. Restores the shape-conforming mask (a flat quad stood in while shape geometry
	// lived only in the removed legacy pg voxel shapes) on top of the spk voxel library.
	[[nodiscard]] spk::TextureMesh3D buildHoverMesh(
		const pg::VoxelWorld &p_world,
		const spk::Vector3Int &p_cell,
		const spk::AtlasCell &p_mask)
	{
		const spk::VoxelCell &cell = p_world.cell(p_cell);
		spk::TextureMesh3D::Builder builder;
		if (cell.isEmpty())
		{
			return builder.bake();
		}

		const spk::VoxelShape &shape = p_world.registry().renderRegistry().shape(cell.id);
		const spk::VoxelShapeFaceSet &faces = shape.renderFaces();
		const spk::Vector3 offset = spk::Vector3(p_cell) + spk::Vector3{0.0f, MaskLift, 0.0f};

		for (std::size_t index = 0; index < faces.innerFaces.size(); ++index)
		{
			appendWalkSurface(builder, shape.transformedInnerFace(index, cell.orientation, cell.flip), offset, p_mask);
		}
		for (std::size_t planeIndex = 0; planeIndex < static_cast<std::size_t>(spk::VoxelAxisPlane::Count); ++planeIndex)
		{
			const auto plane = static_cast<spk::VoxelAxisPlane>(planeIndex);
			if (faces.outer(plane).has_value())
			{
				appendWalkSurface(builder, shape.transformedOuterFace(plane, cell.orientation, cell.flip), offset, p_mask);
			}
		}
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
		_hoverRenderer.setMesh(std::make_shared<spk::TextureMesh3D>(buildHoverMesh(_world, *_hovered, mask)));
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
