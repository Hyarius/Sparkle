#include "support/board_fixture.hpp"

#include "voxel/voxel_registry.hpp"

#include <atomic>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <stdexcept>

namespace
{
	class LiteralVoxelDirectory
	{
	private:
		std::filesystem::path _path;

		void _write(const std::string &p_name, const std::string &p_json)
		{
			std::ofstream stream(_path / (p_name + ".json"));
			stream << p_json;
		}

	public:
		LiteralVoxelDirectory()
		{
			static std::atomic<unsigned int> counter = 0;
			_path = std::filesystem::temp_directory_path() /
					("sparkle-board-fixture-" +
					 std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()) + "-" +
					 std::to_string(counter++));
			std::filesystem::create_directories(_path);
			_write("stone-block", R"({"version":1,"traversal":"solid","tags":["ground"],"shape":{"type":"cube","textures":{"top":[0,0],"bottom":[0,0],"side":[0,0]}}})");
			_write("wall-stone", R"({"version":1,"traversal":"solid","tags":["wall"],"shape":{"type":"cube","textures":{"top":[0,0],"bottom":[0,0],"side":[0,0]}}})");
			_write("slope-grass", R"({"version":1,"traversal":"solid","tags":["ground","slope"],"shape":{"type":"slope","textures":{"slope":[0,0],"back":[0,0],"bottom":[0,0],"sideLeft":[0,0],"sideRight":[0,0]}}})");
			_write("stair-stone", R"({"version":1,"traversal":"solid","tags":["ground","stair"],"shape":{"type":"stair","stepCount":2,"textures":{"top":[0,0],"riser":[0,0],"back":[0,0],"bottom":[0,0],"sideLeft":[0,0],"sideRight":[0,0]}}})");
			_write("slab-stone", R"({"version":1,"traversal":"solid","tags":["ground"],"shape":{"type":"slab","height":0.5,"textures":{"top":[0,0],"bottom":[0,0],"side":[0,0]}}})");
			_write("bush", R"({"version":1,"traversal":"passable","tags":["bush","losTransparent"],"shape":{"type":"crossPlane","textures":{"plane":[0,0]}}})");
		}

		~LiteralVoxelDirectory()
		{
			std::error_code error;
			std::filesystem::remove_all(_path, error);
		}

		[[nodiscard]] const std::filesystem::path &path() const noexcept
		{
			return _path;
		}
	};

	[[nodiscard]] pg::VoxelCell voxel(
		const pg::VoxelRegistry &p_registry,
		const std::string &p_id,
		pg::VoxelOrientation p_orientation = pg::VoxelOrientation::PositiveZ)
	{
		return {p_registry.numericId(p_id), p_orientation};
	}
}

namespace pg::test
{
	const VoxelRegistry &BoardFixture::registry()
	{
		static const VoxelRegistry result = [] {
			LiteralVoxelDirectory directory;
			VoxelRegistry loaded;
			loaded.load(directory.path());
			return loaded;
		}();
		return result;
	}

	BoardFixture::BoardFixture(
		const std::vector<std::string> &p_rows,
		std::map<std::string, spk::Vector3Int> p_namedCells) :
		BoardFixture(std::vector<std::vector<std::string>>{p_rows}, std::move(p_namedCells))
	{
	}

	BoardFixture::BoardFixture(
		const std::vector<std::vector<std::string>> &p_layers,
		std::map<std::string, spk::Vector3Int> p_namedCells) :
		_namedCells(std::move(p_namedCells))
	{
		if (p_layers.empty() || p_layers.front().empty() || p_layers.front().front().empty())
		{
			throw std::invalid_argument("BoardFixture layers must not be empty");
		}
		const int depth = static_cast<int>(p_layers.front().size());
		const int width = static_cast<int>(p_layers.front().front().size());
		for (const std::vector<std::string> &layer : p_layers)
		{
			if (static_cast<int>(layer.size()) != depth)
			{
				throw std::invalid_argument("BoardFixture layers must have equal depth");
			}
			for (const std::string &row : layer)
			{
				if (static_cast<int>(row.size()) != width)
				{
					throw std::invalid_argument("BoardFixture rows must have equal width");
				}
			}
		}
		_grid = VoxelGrid({width, static_cast<int>(p_layers.size()) + 4, depth});
		for (int y = 0; y < static_cast<int>(p_layers.size()); ++y)
		{
			for (int z = 0; z < depth; ++z)
			{
				for (int x = 0; x < width; ++x)
				{
					const char symbol = p_layers[static_cast<std::size_t>(y)][static_cast<std::size_t>(z)][static_cast<std::size_t>(x)];
					switch (symbol)
					{
					case '.':
						break;
					case '#':
						_grid.cell(x, y, z) = voxel(registry(), "stone-block");
						break;
					case '/':
						_grid.cell(x, y, z) = voxel(registry(), "slope-grass", VoxelOrientation::PositiveZ);
						break;
					case '\\':
						_grid.cell(x, y, z) = voxel(registry(), "slope-grass", VoxelOrientation::NegativeZ);
						break;
					case '<':
						_grid.cell(x, y, z) = voxel(registry(), "slope-grass", VoxelOrientation::NegativeX);
						break;
					case '>':
						_grid.cell(x, y, z) = voxel(registry(), "slope-grass", VoxelOrientation::PositiveX);
						break;
					case 's':
						_grid.cell(x, y, z) = voxel(registry(), "stair-stone");
						break;
					case '_':
						_grid.cell(x, y, z) = voxel(registry(), "slab-stone");
						break;
					case 'b':
						_grid.cell(x, y, z) = voxel(registry(), "stone-block");
						_grid.cell(x, y + 1, z) = voxel(registry(), "bush");
						break;
					case 'x':
						_grid.cell(x, y, z) = voxel(registry(), "stone-block");
						_grid.cell(x, y + 1, z) = voxel(registry(), "wall-stone");
						_grid.cell(x, y + 2, z) = voxel(registry(), "wall-stone");
						break;
					default:
						throw std::invalid_argument("Unknown BoardFixture symbol");
					}
				}
			}
		}
		_board = std::make_unique<BoardData>(BoardBuilder::fromGrid(_grid, registry()));
	}

	const VoxelGrid &BoardFixture::grid() const noexcept
	{
		return _grid;
	}
	BoardData &BoardFixture::board() noexcept
	{
		return *_board;
	}
	const BoardData &BoardFixture::board() const noexcept
	{
		return *_board;
	}
	const TraversalGraph &BoardFixture::graph() const noexcept
	{
		return _board->navigation().graph();
	}

	spk::Vector3Int BoardFixture::cell(int p_x, int p_z) const
	{
		for (int y = _grid.size().y - 1; y >= 0; --y)
		{
			const spk::Vector3Int candidate{p_x, y, p_z};
			if (_board->isStandable(candidate))
			{
				return candidate;
			}
		}
		throw std::out_of_range("BoardFixture column has no standable cell");
	}

	spk::Vector3Int BoardFixture::cell(const std::string &p_name) const
	{
		const auto cell = _namedCells.find(p_name);
		if (cell == _namedCells.end())
		{
			throw std::out_of_range("Unknown BoardFixture cell name '" + p_name + "'");
		}
		return cell->second;
	}

	void BoardFixture::name(std::string p_name, const spk::Vector3Int &p_cell)
	{
		if (!_board->isStandable(p_cell))
		{
			throw std::invalid_argument("Named fixture cell must be standable");
		}
		_namedCells.insert_or_assign(std::move(p_name), p_cell);
	}
}
