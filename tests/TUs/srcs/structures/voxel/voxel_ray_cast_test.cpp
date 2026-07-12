#include <gtest/gtest.h>

#include "structures/voxel/spk_voxel_ray_cast.hpp"

#include <limits>
#include <map>
#include <tuple>

namespace
{
	struct PositionLess
	{
		bool operator()(const spk::Vector3Int &p_left, const spk::Vector3Int &p_right) const noexcept
		{
			return std::tie(p_left.x, p_left.y, p_left.z) < std::tie(p_right.x, p_right.y, p_right.z);
		}
	};

	class Cells final : public spk::IVoxelCellLookup
	{
	private:
		std::map<spk::Vector3Int, spk::VoxelCell, PositionLess> _cells;

	public:
		void set(const spk::Vector3Int &p_position, spk::VoxelRuntimeId p_id)
		{
			_cells[p_position] = {.id = p_id};
		}

		const spk::VoxelCell *tryCell(const spk::Vector3Int &p_position) const override
		{
			const auto iterator = _cells.find(p_position);
			return iterator == _cells.end() ? nullptr : &iterator->second;
		}
	};
}

TEST(VoxelRayCast, HitsTheBoundaryOfTheFirstNonEmptyVoxel)
{
	Cells cells;
	cells.set({2, 0, 0}, spk::VoxelRuntimeId{7});

	const auto hit = spk::VoxelRayCast::cast(cells, {{0.25f, 0.5f, 0.5f}, {2, 0, 0}}, 4.0f);
	ASSERT_TRUE(hit.has_value());
	EXPECT_EQ(hit->cell, spk::Vector3Int(2, 0, 0));
	EXPECT_NEAR(hit->distance, 1.75f, 0.0001f);
	EXPECT_EQ(hit->entryNormal, spk::Vector3Int(-1, 0, 0));
}

TEST(VoxelRayCast, TreatsEveryVoxelAsAUnitBoxAndSupportsSelectionPolicy)
{
	Cells cells;
	cells.set({1, 0, 0}, spk::VoxelRuntimeId{1});
	cells.set({2, 0, 0}, spk::VoxelRuntimeId{2});

	const auto hit = spk::VoxelRayCast::cast(
		cells,
		{{0.5f, 0.9f, 0.5f}, {1, 0, 0}},
		4.0f,
		[](const spk::Vector3Int &, const spk::VoxelCell &p_cell) { return p_cell.id == spk::VoxelRuntimeId{2}; });
	ASSERT_TRUE(hit.has_value());
	EXPECT_EQ(hit->cell, spk::Vector3Int(2, 0, 0));
	EXPECT_NEAR(hit->distance, 1.5f, 0.0001f);
}

TEST(VoxelRayCast, ReportsImmediateHitWhenStartingInsideASelectedCell)
{
	Cells cells;
	cells.set({1, 0, 0}, spk::VoxelRuntimeId{1});

	const auto hit = spk::VoxelRayCast::cast(cells, {{1.5f, 0.25f, 0.5f}, {1, 0, 0}}, 2.0f);
	ASSERT_TRUE(hit.has_value());
	EXPECT_FLOAT_EQ(hit->distance, 0.0f);
	EXPECT_EQ(hit->entryNormal, spk::Vector3Int(0, 0, 0));
}

TEST(VoxelRayCast, AdvancesEveryAxisAtEdgeAndCornerTies)
{
	Cells cells;
	cells.set({1, 1, 0}, spk::VoxelRuntimeId{1});
	cells.set({1, 1, 1}, spk::VoxelRuntimeId{2});

	const auto edge = spk::VoxelRayCast::cast(cells, {{0.5f, 0.5f, 0.5f}, {1, 1, 0}}, 4.0f);
	ASSERT_TRUE(edge.has_value());
	EXPECT_EQ(edge->cell, spk::Vector3Int(1, 1, 0));
	EXPECT_EQ(edge->entryNormal, spk::Vector3Int(-1, -1, 0));

	const auto corner = spk::VoxelRayCast::cast(
		cells,
		{{0.5f, 0.5f, 0.5f}, {1, 1, 1}},
		4.0f,
		[](const spk::Vector3Int &, const spk::VoxelCell &p_cell) { return p_cell.id == spk::VoxelRuntimeId{2}; });
	ASSERT_TRUE(corner.has_value());
	EXPECT_EQ(corner->cell, spk::Vector3Int(1, 1, 1));
	EXPECT_EQ(corner->entryNormal, spk::Vector3Int(-1, -1, -1));
}

TEST(VoxelRayCast, ValidatesInputsAndHonorsMaximumDistance)
{
	Cells cells;
	cells.set({2, 0, 0}, spk::VoxelRuntimeId{1});

	EXPECT_FALSE(spk::VoxelRayCast::cast(cells, {{0, 0.5f, 0.5f}, {1, 0, 0}}, 1.99f));
	EXPECT_TRUE(spk::VoxelRayCast::cast(cells, {{0, 0.5f, 0.5f}, {1, 0, 0}}, 2.0f));
	EXPECT_FALSE(spk::VoxelRayCast::cast(cells, {{0, 0, 0}, {1, 0, 0}}, -1.0f));
	EXPECT_THROW((void)spk::VoxelRayCast::cast(cells, {{0, 0, 0}, {0, 0, 0}}, 1.0f), std::invalid_argument);
	EXPECT_THROW(
		(void)spk::VoxelRayCast::cast(cells, {{std::numeric_limits<float>::quiet_NaN(), 0, 0}, {1, 0, 0}}, 1.0f),
		std::invalid_argument);
	EXPECT_THROW(
		(void)spk::VoxelRayCast::cast(cells, {{0, 0, 0}, {1, 0, 0}}, std::numeric_limits<float>::infinity()),
		std::invalid_argument);
	EXPECT_THROW(
		(void)spk::VoxelRayCast::cast(cells, {{std::numeric_limits<float>::max(), 0, 0}, {1, 0, 0}}, 1.0f),
		std::out_of_range);
}
