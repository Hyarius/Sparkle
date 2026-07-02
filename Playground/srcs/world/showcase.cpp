#include "world/showcase.hpp"

#include "voxel/voxel_registry.hpp"

#include <array>
#include <cstdint>
#include <functional>
#include <string>

namespace
{
	using IdResolver = std::function<std::int32_t(const std::string &)>;

	[[nodiscard]] pg::VoxelGrid build(const IdResolver &p_id)
	{
		pg::VoxelGrid grid({12, 4, 12});
		const std::int32_t bush = p_id("bush");
		const std::int32_t dirt = p_id("dirt-block");
		const std::int32_t grass = p_id("grass-block");
		const std::int32_t slab = p_id("slab-stone");
		const std::int32_t slope = p_id("slope-grass");
		const std::int32_t stair = p_id("stair-stone");
		const std::int32_t stone = p_id("stone-block");
		const std::int32_t wall = p_id("wall-stone");

		// A complete ground plane makes boundary and top-face culling immediately visible.
		for (int z = 0; z < grid.size().z; ++z)
		{
			for (int x = 0; x < grid.size().x; ++x)
			{
				grid.cell(x, 0, z) = {((x + z) % 5 == 0) ? dirt : grass};
			}
		}

		// A 3x3x3 block has one fully buried center voxel and exercises interior culling.
		for (int y = 1; y <= 3; ++y)
		{
			for (int z = 0; z < 3; ++z)
			{
				for (int x = 0; x < 3; ++x)
				{
					grid.cell(x, y, z) = {stone};
				}
			}
		}

		const std::array orientations = {
			pg::VoxelOrientation::PositiveZ,
			pg::VoxelOrientation::PositiveX,
			pg::VoxelOrientation::NegativeZ,
			pg::VoxelOrientation::NegativeX};
		for (std::size_t index = 0; index < orientations.size(); ++index)
		{
			const int x = 4 + static_cast<int>(index);
			grid.cell(x, 1, 1) = {slope, orientations[index]};
			grid.cell(x, 1, 3) = {stair, orientations[index]};
			grid.cell(x, 1, 5) = {slab, orientations[index]};
			grid.cell(x, 1, 7) = {bush, orientations[index]};
			grid.cell(x, 1, 9) = {wall, orientations[index]};
		}

		// Short ascending chains demonstrate that slope/stair surfaces meet the next layer.
		grid.cell(9, 1, 1) = {slope, pg::VoxelOrientation::PositiveZ};
		grid.cell(9, 2, 2) = {slope, pg::VoxelOrientation::PositiveZ};
		grid.cell(10, 1, 1) = {stair, pg::VoxelOrientation::PositiveZ};
		grid.cell(10, 2, 2) = {stair, pg::VoxelOrientation::PositiveZ};

		for (int x = 4; x <= 8; ++x)
		{
			grid.cell(x, 1, 11) = {slab};
		}
		grid.cell(8, 1, 7) = {bush};
		grid.cell(9, 1, 7) = {bush, pg::VoxelOrientation::PositiveX};

		// Mirroring the slope in Y creates a ceiling wedge directly under the overhang.
		grid.cell(10, 1, 8) = {
			slope,
			pg::VoxelOrientation::NegativeX,
			pg::VoxelFlip::NegativeY};
		grid.cell(10, 2, 8) = {wall};
		grid.cell(9, 2, 8) = {wall};
		grid.cell(11, 2, 8) = {wall};

		return grid;
	}
}

namespace pg
{
	VoxelGrid buildShowcaseGrid()
	{
		// Alphabetical order: bush, dirt, grass, slab, slope, stair, stone, wall.
		return build([](const std::string &p_id) -> std::int32_t {
			if (p_id == "bush") return 0;
			if (p_id == "dirt-block") return 1;
			if (p_id == "grass-block") return 2;
			if (p_id == "slab-stone") return 3;
			if (p_id == "slope-grass") return 4;
			if (p_id == "stair-stone") return 5;
			if (p_id == "stone-block") return 6;
			if (p_id == "wall-stone") return 7;
			return VoxelCell::EmptyId;
		});
	}

	VoxelGrid buildShowcaseGrid(const VoxelRegistry &p_registry)
	{
		return build([&p_registry](const std::string &p_id) {
			return p_registry.numericId(p_id);
		});
	}
}
