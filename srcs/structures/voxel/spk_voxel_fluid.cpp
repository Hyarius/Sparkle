#include "structures/voxel/spk_voxel_fluid.hpp"

#include "structures/voxel/spk_data_voxel_shape.hpp"

#include <stdexcept>
#include <string>
#include <vector>

namespace spk
{
	std::size_t VoxelFluidFamily::levelCount() const noexcept
	{
		return levels.size();
	}

	const VoxelFluidState &VoxelFluidFamily::level(std::size_t p_level) const
	{
		if (p_level < 1 || p_level > levels.size())
		{
			throw std::out_of_range(
				"Fluid family has no level " + std::to_string(p_level) + " (levels run 1.." +
				std::to_string(levels.size()) + ")");
		}
		return levels[p_level - 1];
	}

	namespace detail
	{
		std::unique_ptr<spk::VoxelShape> makeVoxelFluidStateShape(
			const spk::VoxelFluidAppearance &p_appearance,
			float p_height)
		{
			if (p_height <= 0.0f || p_height > 1.0f)
			{
				throw std::invalid_argument("Fluid fill height must be within (0, 1]");
			}

			const float h = p_height;
			const std::vector<spk::Vector2> horizontalUVs = {{0, 0}, {1, 0}, {1, 1}, {0, 1}};
			// Vertical faces run bottom edge first: crop the V range to the fill height.
			const std::vector<spk::Vector2> sideUVs = {{0, 1}, {1, 1}, {1, 1.0f - h}, {0, 1.0f - h}};

			// The top of a partial level sits below the boundary, so DataVoxelShape classifies
			// it as an inner face (a buried level culls it, an exposed one still renders it);
			// the bottom and the four boundary sides join the outer shell and take part in the
			// mesher's partial-occlusion coverage math.
			spk::VoxelShapeDescription description;
			description.polygons = {
				{"top", {{0, h, 1}, {1, h, 1}, {1, h, 0}, {0, h, 0}}, horizontalUVs},
				{"bottom", {{0, 0, 0}, {1, 0, 0}, {1, 0, 1}, {0, 0, 1}}, horizontalUVs},
				{"side", {{1, 0, 1}, {1, 0, 0}, {1, h, 0}, {1, h, 1}}, sideUVs},
				{"side", {{0, 0, 0}, {0, 0, 1}, {0, h, 1}, {0, h, 0}}, sideUVs},
				{"side", {{0, 0, 1}, {1, 0, 1}, {1, h, 1}, {0, h, 1}}, sideUVs},
				{"side", {{1, 0, 0}, {0, 0, 0}, {0, h, 0}, {1, h, 0}}, sideUVs},
			};

			return std::make_unique<spk::DataVoxelShape>(
				spk::VoxelShape::TextureSlots{
					{"top", p_appearance.topTexture},
					{"bottom", p_appearance.bottomTexture},
					{"side", p_appearance.sideTexture}},
				std::move(description),
				p_appearance.atlasSize);
		}
	}
}
