#include "fixtures/board_fixture.hpp"

#include "core/json.hpp"
#include "core/paths.hpp"

#include <array>
#include <filesystem>
#include <fstream>
#include <optional>
#include <stdexcept>
#include <string_view>
#include <utility>

namespace pgtest
{
	namespace
	{
		// One authored voxel per fixture symbol. They are written to disk and read back through the
		// real parser and the real shape catalog, so a fixture board cannot drift away from the rules
		// production content obeys - a movement cost or a LoS tag it gets wrong here would be a bug it
		// hides everywhere else.
		struct FixtureVoxel
		{
			const char *id;
			const char *json;
		};

		constexpr std::array<FixtureVoxel, 9> FixtureVoxels{{
			{"test-ground",
			 R"({"version": 2, "traversal": "solid", "tags": ["ground"], "shape": "cube",
				 "textures": {"top": [0, 0], "bottom": [0, 0], "side": [0, 0]}})"},
			{"test-mud",
			 R"({"version": 2, "traversal": "solid", "tags": ["ground"], "shape": "cube", "movementCost": 2,
				 "textures": {"top": [0, 0], "bottom": [0, 0], "side": [0, 0]}})"},
			{"test-deep-mud",
			 R"({"version": 2, "traversal": "solid", "tags": ["ground"], "shape": "cube", "movementCost": 3,
				 "textures": {"top": [0, 0], "bottom": [0, 0], "side": [0, 0]}})"},
			{"test-wall",
			 R"({"version": 2, "traversal": "solid", "tags": ["wall"], "shape": "cube",
				 "textures": {"top": [0, 0], "bottom": [0, 0], "side": [0, 0]}})"},
			// Solid, so it blocks movement, yet explicitly see-through for targeting: the two are
			// separate properties and this voxel exists to prove it.
			{"test-glass",
			 R"({"version": 2, "traversal": "solid", "tags": ["wall", "losTransparent"], "shape": "cube",
				 "textures": {"top": [0, 0], "bottom": [0, 0], "side": [0, 0]}})"},
			{"test-bush",
			 R"({"version": 2, "traversal": "passable", "tags": ["flora", "losTransparent"], "shape": "cross-plane",
				 "textures": {"plane": [0, 0]}})"},
			{"test-slab",
			 R"({"version": 2, "traversal": "solid", "tags": ["ground"], "shape": "slab",
				 "textures": {"top": [0, 0], "bottom": [0, 0], "side": [0, 0]}})"},
			{"test-slope",
			 R"({"version": 2, "traversal": "solid", "tags": ["ground", "slope"], "shape": "slope",
				 "textures": {"slope": [0, 0], "back": [0, 0], "bottom": [0, 0], "sideLeft": [0, 0], "sideRight": [0, 0]}})"},
			{"test-stair",
			 R"({"version": 2, "traversal": "solid", "tags": ["ground", "stair"], "shape": "stair",
				 "textures": {"top": [0, 0], "riser": [0, 0], "back": [0, 0], "bottom": [0, 0], "sideLeft": [0, 0], "sideRight": [0, 0]}})"},
		}};

		struct Symbol
		{
			std::string voxelId;
			pg::VoxelOrientation orientation = pg::VoxelOrientation::PositiveZ;
		};

		[[nodiscard]] std::optional<Symbol> symbolOf(char p_symbol)
		{
			switch (p_symbol)
			{
			case '.':
				return std::nullopt;
			case '#':
				return Symbol{.voxelId = "test-ground"};
			case '2':
				return Symbol{.voxelId = "test-mud"};
			case '3':
				return Symbol{.voxelId = "test-deep-mud"};
			case 'b':
				return Symbol{.voxelId = "test-bush"};
			case 'W':
				return Symbol{.voxelId = "test-wall"};
			case 'G':
				return Symbol{.voxelId = "test-glass"};
			case '_':
				return Symbol{.voxelId = "test-slab"};
			case 's':
				return Symbol{.voxelId = "test-stair", .orientation = pg::VoxelOrientation::PositiveZ};
			case '/':
				return Symbol{.voxelId = "test-slope", .orientation = pg::VoxelOrientation::PositiveZ};
			case '\\':
				return Symbol{.voxelId = "test-slope", .orientation = pg::VoxelOrientation::NegativeZ};
			case '>':
				return Symbol{.voxelId = "test-slope", .orientation = pg::VoxelOrientation::PositiveX};
			case '<':
				return Symbol{.voxelId = "test-slope", .orientation = pg::VoxelOrientation::NegativeX};
			default:
				throw std::invalid_argument(std::string("unknown board fixture symbol '") + p_symbol + "'");
			}
		}

		// Written once per process, next to nothing else: the fixture never depends on the graphical
		// executable's working directory, and its voxels never leak into the shipped content tree.
		[[nodiscard]] const std::filesystem::path &fixtureVoxelDirectory()
		{
			static const std::filesystem::path directory = [] {
				const std::filesystem::path path =
					std::filesystem::temp_directory_path() / "sparkle-playground-board-fixture" / "voxels";
				std::filesystem::remove_all(path);
				std::filesystem::create_directories(path);
				for (const FixtureVoxel &voxel : FixtureVoxels)
				{
					std::ofstream stream(path / (std::string(voxel.id) + ".json"), std::ios::binary | std::ios::trunc);
					stream << voxel.json;
				}
				return path;
			}();
			return directory;
		}

		// One layer's rows, split on '\n' and checked into a rectangle.
		[[nodiscard]] std::vector<std::string> rowsOf(const std::string &p_layer)
		{
			std::vector<std::string> rows;
			std::string row;
			for (const char character : p_layer)
			{
				if (character == '\n')
				{
					rows.push_back(row);
					row.clear();
					continue;
				}
				if (character != '\r')
				{
					row.push_back(character);
				}
			}
			rows.push_back(row);
			return rows;
		}
	}

	void loadFixtureVoxels(pg::ShapeCatalog &p_shapes, pg::VoxelFamilyCatalog &p_families, pg::VoxelRegistry &p_voxels)
	{
		spk::loadJsonDirectory(
			p_shapes, pg::resourceRoot() / "data" / "shapes", [](std::string_view p_id, pg::JsonReader &p_reader) {
				pg::ShapeDefinition definition = pg::parseShapeDefinition(p_reader);
				definition.id = p_id;
				return definition;
			});
		p_voxels.load(p_shapes, p_families, fixtureVoxelDirectory());
	}

	std::shared_ptr<const spk::VoxelGrid> gridFromLayers(
		const std::vector<std::string> &p_layers,
		const pg::VoxelRegistry &p_voxels)
	{
		if (p_layers.empty())
		{
			throw std::invalid_argument("a board fixture needs at least one layer");
		}

		std::vector<std::vector<std::string>> layers;
		layers.reserve(p_layers.size());
		for (const std::string &layer : p_layers)
		{
			layers.push_back(rowsOf(layer));
		}

		const std::size_t depth = layers.front().size();
		const std::size_t width = layers.front().front().size();
		if (width == 0 || depth == 0)
		{
			throw std::invalid_argument("a board fixture layer is a non-empty rectangle");
		}
		for (const std::vector<std::string> &layer : layers)
		{
			if (layer.size() != depth)
			{
				throw std::invalid_argument("every board fixture layer has the same number of rows");
			}
			for (const std::string &row : layer)
			{
				if (row.size() != width)
				{
					throw std::invalid_argument("every board fixture row has the same length");
				}
			}
		}

		const auto grid = std::make_shared<spk::VoxelGrid>(spk::Vector3Int{
			static_cast<int>(width), static_cast<int>(layers.size()), static_cast<int>(depth)});
		for (std::size_t y = 0; y < layers.size(); ++y)
		{
			for (std::size_t z = 0; z < depth; ++z)
			{
				for (std::size_t x = 0; x < width; ++x)
				{
					const std::optional<Symbol> symbol = symbolOf(layers[y][z][x]);
					if (!symbol.has_value())
					{
						continue;
					}
					grid->cell({static_cast<int>(x), static_cast<int>(y), static_cast<int>(z)}) = spk::VoxelCell{
						.id = p_voxels.runtimeId(symbol->voxelId), .orientation = symbol->orientation};
				}
			}
		}
		return grid;
	}

	BoardFixture::Request BoardFixture::flat(
		int p_width,
		int p_depth,
		int p_deploymentDepth,
		std::size_t p_playerSeatCount,
		std::size_t p_enemySeatCount)
	{
		const auto layer = [p_width, p_depth](char p_symbol) {
			std::string result;
			for (int z = 0; z < p_depth; ++z)
			{
				if (z != 0)
				{
					result += '\n';
				}
				result += std::string(static_cast<std::size_t>(p_width), p_symbol);
			}
			return result;
		};

		return Request{
			.layers = {layer('#'), layer('.'), layer('.')},
			.deploymentDepth = p_deploymentDepth,
			.playerSeatCount = p_playerSeatCount,
			.enemySeatCount = p_enemySeatCount};
	}

	BoardFixture::BoardFixture(Request p_request) :
		_namedCells(std::move(p_request.namedCells))
	{
		loadFixtureVoxels(_shapes, _families, _voxels);

		const std::shared_ptr<const spk::VoxelGrid> grid = gridFromLayers(p_request.layers, _voxels);
		const spk::Vector3Int size = grid->size();

		// The exact path a handcrafted arena takes - an owned immutable grid through
		// buildFrozenSource - so what a later battle test asserts against a fixture is what it would
		// have got from a registered board.
		_result = pg::BoardBuilder::buildFrozenSource(
			pg::BoardSourceDescriptor{
				.kind = pg::BoardSourceKind::Handcrafted, .definitionId = p_request.definitionId},
			std::make_shared<const pg::GridCellSource>(grid, _voxels),
			pg::BoardExtent{
				.size = {size.x, size.z},
				.traversalBounds = pg::TraversalBounds{.minimum = {0, 0, 0}, .maximum = size}},
			spk::Vector3Int{},
			std::nullopt,
			std::nullopt,
			p_request.playerApproach,
			p_request.deploymentDepth,
			p_request.playerSeatCount,
			p_request.enemySeatCount,
			p_request.maxVerticalGap);
	}

	bool BoardFixture::built() const noexcept
	{
		return _result.board.has_value();
	}

	const pg::BoardBuildError &BoardFixture::error() const
	{
		if (!_result.error.has_value())
		{
			throw std::logic_error("this board fixture built successfully: it has no error to report");
		}
		return *_result.error;
	}

	pg::BoardData &BoardFixture::board()
	{
		if (!_result.board.has_value())
		{
			throw std::logic_error(
				"the board fixture failed to build: " + std::string(pg::toString(_result.error->code)) + " (" +
				_result.error->diagnosticDetail + ")");
		}
		return *_result.board;
	}

	const pg::BoardData &BoardFixture::board() const
	{
		return const_cast<BoardFixture *>(this)->board();
	}

	pg::BoardMutation BoardFixture::mutation()
	{
		return pg::BoardMutation(board());
	}

	const pg::VoxelRegistry &BoardFixture::voxels() const noexcept
	{
		return _voxels;
	}

	pg::BoardCell BoardFixture::cell(const std::string &p_name) const
	{
		const auto found = _namedCells.find(p_name);
		if (found == _namedCells.end())
		{
			throw std::out_of_range("no cell named '" + p_name + "' was bound on this board fixture");
		}
		return found->second;
	}

	void BoardFixture::placeUnit(pg::BattleUnitId p_unit, const pg::BoardCell &p_cell)
	{
		const pg::BoardMutationResult result = mutation().placeUnit(p_unit, p_cell);
		if (!result.accepted)
		{
			throw std::logic_error("the board fixture could not place a unit on " + p_cell.toString());
		}
	}

	void BoardFixture::placeObject(pg::BattleObjectId p_object, const pg::BoardCell &p_cell)
	{
		const pg::BoardMutationResult result = mutation().placeObject(p_object, p_cell);
		if (!result.accepted)
		{
			throw std::logic_error("the board fixture could not place an object on " + p_cell.toString());
		}
	}
}
