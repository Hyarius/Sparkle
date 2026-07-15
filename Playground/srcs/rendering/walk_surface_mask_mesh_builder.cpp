#include "rendering/walk_surface_mask_mesh_builder.hpp"

#include "structures/voxel/spk_voxel_shape.hpp"

#include <algorithm>
#include <stdexcept>
#include <vector>

namespace
{
	[[nodiscard]] spk::Vector2 maskUV(
		const pg::MaskAtlasLayout &p_layout,
		const spk::AtlasCell &p_cell,
		float p_u,
		float p_v)
	{
		return {
			(static_cast<float>(p_cell.column) + p_u) / static_cast<float>(p_layout.columns),
			(static_cast<float>(p_cell.row) + p_v) / static_cast<float>(p_layout.rows)};
	}

	void appendWalkSurface(
		spk::TextureMesh3D::Builder &p_builder,
		const spk::VoxelShapeFace &p_face,
		const spk::Vector3 &p_offset,
		const spk::AtlasCell &p_mask,
		const pg::MaskAtlasLayout &p_layout)
	{
		for (const spk::VoxelShapePolygon &polygon : p_face.polygons())
		{
			if (polygon.size() < 3 || polygon.normal().y <= 0.0001f)
			{
				continue;
			}
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
				vertices.push_back({
					vertex.position + p_offset,
					polygon.normal(),
					maskUV(p_layout, p_mask, localU, localV)});
			}
			p_builder.addShape(vertices);
		}
	}

	void appendInstance(
		spk::TextureMesh3D::Builder &p_builder,
		const pg::IBattlePresentationCellSource &p_cells,
		const pg::WalkSurfaceMaskInstance &p_instance,
		const pg::MaskAtlasLayout &p_layout)
	{
		const spk::VoxelCell *cell = p_cells.tryCell(p_instance.supportCell);
		const spk::VoxelShape *shape = cell == nullptr ? nullptr : p_cells.tryRenderShape(*cell);
		if (shape == nullptr)
		{
			return;
		}
		const spk::VoxelShapeFaceSet &faces = shape->renderFaces();
		const spk::Vector3 offset = spk::Vector3(p_instance.supportCell) + spk::Vector3{0.0f, p_instance.lift, 0.0f};
		for (std::size_t index = 0; index < faces.innerFaces.size(); ++index)
		{
			appendWalkSurface(
				p_builder,
				shape->transformedInnerFace(index, cell->orientation, cell->flip),
				offset,
				p_instance.atlasCell,
				p_layout);
		}
		for (std::size_t index = 0; index < static_cast<std::size_t>(spk::VoxelAxisPlane::Count); ++index)
		{
			const auto plane = static_cast<spk::VoxelAxisPlane>(index);
			if (faces.outer(plane).has_value())
			{
				appendWalkSurface(
					p_builder,
					shape->transformedOuterFace(plane, cell->orientation, cell->flip),
					offset,
					p_instance.atlasCell,
					p_layout);
			}
		}
	}
}

namespace pg
{
	bool MaskAtlasLayout::contains(const spk::AtlasCell &p_cell) const noexcept
	{
		return columns > 0 && rows > 0 && p_cell.column >= 0 && p_cell.column < columns &&
			   p_cell.row >= 0 && p_cell.row < rows;
	}

	spk::TextureMesh3D WalkSurfaceMaskMeshBuilder::build(
		const IBattlePresentationCellSource &p_cells,
		std::span<const WalkSurfaceMaskInstance> p_instances,
		MaskAtlasLayout p_atlas)
	{
		if (p_atlas.columns <= 0 || p_atlas.rows <= 0)
		{
			throw std::invalid_argument("mask atlas layout must have positive dimensions");
		}
		spk::TextureMesh3D::Builder builder;
		for (const WalkSurfaceMaskInstance &instance : p_instances)
		{
			if (!p_atlas.contains(instance.atlasCell))
			{
				throw std::out_of_range("walk-surface mask atlas cell is outside its layout");
			}
			appendInstance(builder, p_cells, instance, p_atlas);
		}
		return builder.bake();
	}

	spk::TextureMesh3D WalkSurfaceMaskMeshBuilder::buildOne(
		const IBattlePresentationCellSource &p_cells,
		const spk::Vector3Int &p_supportCell,
		const spk::AtlasCell &p_atlasCell,
		float p_lift,
		MaskAtlasLayout p_atlas)
	{
		const WalkSurfaceMaskInstance instance{.supportCell = p_supportCell, .atlasCell = p_atlasCell, .lift = p_lift};
		return build(p_cells, std::span<const WalkSurfaceMaskInstance>(&instance, 1), p_atlas);
	}
}
