#include "structures/voxel/spk_voxel_chunk_renderer.hpp"

#include <stdexcept>
#include <utility>

namespace spk
{
	VoxelChunkRenderer::VoxelChunkRenderer(
		const spk::VoxelGrid &p_grid,
		const spk::VoxelRegistry &p_registry,
		const spk::IVoxelCellLookup *p_worldLookup,
		const spk::Vector3Int &p_worldOrigin) :
		_grid(&p_grid),
		_registry(&p_registry),
		_worldLookup(p_worldLookup),
		_worldOrigin(p_worldOrigin)
	{
	}

	void VoxelChunkRenderer::_synchronize() const
	{
		publishMesh(buildMesh());
	}

	bool VoxelChunkRenderer::beginMeshSynchronization() const noexcept
	{
		return _beginSynchronization();
	}

	VoxelChunkRenderer::MeshHandle VoxelChunkRenderer::buildMesh() const
	{
		return std::make_shared<spk::TextureMesh3D>(
			_worldLookup != nullptr
				? spk::VoxelMesher::buildRenderMesh(*_grid, *_registry, *_worldLookup, _worldOrigin)
				: spk::VoxelMesher::buildRenderMesh(*_grid, *_registry));
	}

	void VoxelChunkRenderer::publishMesh(MeshHandle p_mesh) const
	{
		if (p_mesh == nullptr)
		{
			throw std::invalid_argument("VoxelChunkRenderer cannot publish a null mesh");
		}
		_mesh = std::move(p_mesh);
		++_meshRevision;
	}

	void VoxelChunkRenderer::failMeshSynchronization() const noexcept
	{
		_failSynchronization();
	}

	const spk::TextureMesh3D &VoxelChunkRenderer::mesh() const noexcept
	{
		return *_mesh;
	}

	std::shared_ptr<const spk::TextureMesh3D> VoxelChunkRenderer::sharedMesh() const noexcept
	{
		return _mesh;
	}

	std::uint64_t VoxelChunkRenderer::meshRevision() const noexcept
	{
		return _meshRevision;
	}
}
