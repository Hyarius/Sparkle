#pragma once

#include "board/board_builder.hpp"
#include "board/board_data.hpp"
#include "voxel/shape_catalog.hpp"
#include "voxel/voxel_family_definition.hpp"
#include "voxel/voxel_registry.hpp"

#include "structures/voxel/spk_voxel_grid.hpp"

#include <cstddef>
#include <map>
#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace pgtest
{
	// A board a later battle test can ask for in three lines, drawn as ASCII.
	//
	// It is a TEST fixture, not an authoring tool and not a third board source: it materialises an
	// owned immutable grid and hands it to the very same BoardBuilder::buildFrozenSource a
	// handcrafted arena goes through, under a stable explicit test definition id (default
	// "test-flat-board") that no registry ever resolves and no production content ever carries.
	//
	// Layers are listed bottom-up: layers[0] is y = 0. Inside a layer, rows are separated by '\n',
	// the first row is z = 0, and the first character of a row is x = 0. Every row of every layer
	// must be the same length, so the board is a box:
	//
	//     .  empty air                      #  solid support, movement cost 1
	//     2  solid support, cost 2          3  solid support, cost 3
	//     b  passable losTransparent bush   W  solid wall, blocks line of sight
	//     G  solid glass: losTransparent, so it blocks movement but never sight
	//     _  stone slab (half height)       s  stair, facing +Z
	//     >  slope facing +X   <  slope facing -X   /  slope facing +Z   \  slope facing -Z
	//
	// Voxel symbols never encode a unit: occupancy is a separate seam, and a test places stable ids
	// through BoardMutation.
	class BoardFixture
	{
	public:
		struct Request
		{
			std::vector<std::string> layers;
			pg::VoxelOrientation playerApproach = pg::VoxelOrientation::PositiveZ;
			int deploymentDepth = 1;
			std::size_t playerSeatCount = 0;
			std::size_t enemySeatCount = 0;
			std::string definitionId = "test-flat-board";
			double maxVerticalGap = 0.5;
			// Bound explicitly by the caller - "player", "enemy", "anchor", whatever the case needs -
			// rather than smuggled into the voxel alphabet.
			std::map<std::string, pg::BoardCell> namedCells;
		};

	private:
		// Declaration order is the lifetime contract: the registry and the shapes it was built from
		// outlive the board that borrows them, because members die bottom-up.
		pg::ShapeCatalog _shapes;
		pg::VoxelFamilyCatalog _families;
		pg::VoxelRegistry _voxels;
		std::map<std::string, pg::BoardCell> _namedCells;
		pg::BoardBuildResult _result;

	public:
		explicit BoardFixture(Request p_request);

		// A flat cost-1 board of the given x/z extent, one solid layer with air above it.
		[[nodiscard]] static Request flat(
			int p_width,
			int p_depth,
			int p_deploymentDepth = 1,
			std::size_t p_playerSeatCount = 0,
			std::size_t p_enemySeatCount = 0);

		[[nodiscard]] bool built() const noexcept;
		[[nodiscard]] const pg::BoardBuildError &error() const;
		// Throws when the build failed, so a case that meant to get a board fails on the spot.
		[[nodiscard]] pg::BoardData &board();
		[[nodiscard]] const pg::BoardData &board() const;
		[[nodiscard]] pg::BoardMutation mutation();
		[[nodiscard]] const pg::VoxelRegistry &voxels() const noexcept;

		[[nodiscard]] pg::BoardCell cell(const std::string &p_name) const;

		// Places a stable unit id, failing the test's expectations loudly rather than silently
		// leaving the board empty.
		void placeUnit(pg::BattleUnitId p_unit, const pg::BoardCell &p_cell);
		void placeObject(pg::BattleObjectId p_object, const pg::BoardCell &p_cell);
	};

	// The tiny validated voxel registry the fixture builds its grids from: real shape geometry from
	// resources/data/shapes, real parsers, authored movement costs and LoS tags. Exposed so a case
	// can name a voxel when it hand-builds a grid instead of drawing one.
	[[nodiscard]] std::shared_ptr<const spk::VoxelGrid> gridFromLayers(
		const std::vector<std::string> &p_layers,
		const pg::VoxelRegistry &p_voxels);
	void loadFixtureVoxels(pg::ShapeCatalog &p_shapes, pg::VoxelFamilyCatalog &p_families, pg::VoxelRegistry &p_voxels);
}
