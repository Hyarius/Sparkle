#include "tools/pages/world_editor_page.hpp"

#include "core/json.hpp"
#include "logics/chunk_render_logic.hpp"
#include "logics/chunk_synchronization_logic.hpp"
#include "rendering/mouse_picker.hpp"
#include "world/map_definition.hpp"
#include "world/prefab_definition.hpp"
#include "world/voxel_world.hpp"

#include <algorithm>
#include <array>
#include <cctype>
#include <cmath>
#include <limits>
#include <stdexcept>
#include <utility>

namespace
{
	[[nodiscard]] bool validId(const std::string &p_id)
	{
		if (p_id.empty() || p_id.front() == '-' || p_id.back() == '-')
		{
			return false;
		}
		bool dash = false;
		for (const unsigned char character : p_id)
		{
			const bool currentDash = character == '-';
			if ((!std::islower(character) && !std::isdigit(character) && !currentDash) || (dash && currentDash))
			{
				return false;
			}
			dash = currentDash;
		}
		return true;
	}

	[[nodiscard]] nlohmann::json vectorJson(const spk::Vector3Int &p_value)
	{
		return nlohmann::json::array({p_value.x, p_value.y, p_value.z});
	}

	[[nodiscard]] spk::Vector3Int jsonVector(const nlohmann::json &p_value)
	{
		return {p_value.at(0).get<int>(), p_value.at(1).get<int>(), p_value.at(2).get<int>()};
	}

	[[nodiscard]] std::string orientationName(pg::VoxelOrientation p_orientation)
	{
		switch (p_orientation)
		{
		case pg::VoxelOrientation::PositiveX: return "positiveX";
		case pg::VoxelOrientation::PositiveZ: return "positiveZ";
		case pg::VoxelOrientation::NegativeX: return "negativeX";
		case pg::VoxelOrientation::NegativeZ: return "negativeZ";
		}
		return "positiveZ";
	}

	[[nodiscard]] int orientationTurns(pg::VoxelOrientation p_orientation)
	{
		switch (p_orientation)
		{
		case pg::VoxelOrientation::PositiveZ: return 0;
		case pg::VoxelOrientation::PositiveX: return 1;
		case pg::VoxelOrientation::NegativeZ: return 2;
		case pg::VoxelOrientation::NegativeX: return 3;
		}
		return 0;
	}

	[[nodiscard]] pg::VoxelOrientation orientationFromTurns(int p_turns)
	{
		static constexpr std::array values = {
			pg::VoxelOrientation::PositiveZ,
			pg::VoxelOrientation::PositiveX,
			pg::VoxelOrientation::NegativeZ,
			pg::VoxelOrientation::NegativeX};
		return values[static_cast<std::size_t>(p_turns % 4)];
	}

	[[nodiscard]] spk::Vector3Int rotatePosition(
		const spk::Vector3Int &p_position,
		const spk::Vector3Int &p_size,
		pg::VoxelOrientation p_orientation)
	{
		switch (p_orientation)
		{
		case pg::VoxelOrientation::PositiveZ: return p_position;
		case pg::VoxelOrientation::PositiveX: return {p_position.z, p_position.y, p_size.x - 1 - p_position.x};
		case pg::VoxelOrientation::NegativeZ: return {p_size.x - 1 - p_position.x, p_position.y, p_size.z - 1 - p_position.z};
		case pg::VoxelOrientation::NegativeX: return {p_size.z - 1 - p_position.z, p_position.y, p_position.x};
		}
		return p_position;
	}
}

namespace pg::tools
{
	void WorldDocument::load(
		const std::filesystem::path &p_file,
		Kind p_kind,
		const Registries &p_registries)
	{
		_kind = p_kind;
		_file = p_file;
		_sourceFile = p_file;
		_id = p_file.stem().string();
		_json = JsonLoader::parseFile(p_file);
		_voxels = &p_registries.voxels();
		JsonReader reader(_json, p_file);
		if (_kind == Kind::Map)
		{
			MapDefinition definition = parseMapDefinition(reader, p_registries.voxels(), p_registries.prefabs());
			_grid = std::move(definition.grid);
		}
		else
		{
			PrefabDefinition definition = parsePrefabDefinition(reader, p_registries.voxels());
			_grid = std::move(definition.grid);
		}
		_dirty = false;
	}

	void WorldDocument::create(
		const std::filesystem::path &p_directory,
		std::string p_id,
		Kind p_kind,
		const VoxelRegistry &p_voxels,
		const spk::Vector3Int &p_size)
	{
		if (!validId(p_id) || p_size.x <= 0 || p_size.y <= 0 || p_size.z <= 0)
		{
			throw std::invalid_argument("Invalid world document id or size");
		}
		_kind = p_kind;
		_id = std::move(p_id);
		_file = p_directory / (_id + ".json");
		_sourceFile.clear();
		_voxels = &p_voxels;
		_grid = VoxelGrid(p_size);
		_json = {
			{"version", 1},
			{"size", vectorJson(p_size)},
			{"palette", {{"0", nullptr}}},
			{"cells", nlohmann::json::array()}};
		if (_kind == Kind::Map)
		{
			_json["stamps"] = nlohmann::json::array();
			_json["markers"] = nlohmann::json::array();
			_json["portals"] = nlohmann::json::array();
			_json["trainers"] = nlohmann::json::array();
			_json["biome"] = "forest";
		}
		else
		{
			_json["anchors"] = nlohmann::json::array();
		}
		_dirty = true;
	}

	WorldDocument::Kind WorldDocument::kind() const noexcept
	{
		return _kind;
	}

	const std::string &WorldDocument::id() const noexcept
	{
		return _id;
	}

	const std::filesystem::path &WorldDocument::file() const noexcept
	{
		return _file;
	}

	const nlohmann::json &WorldDocument::json() const noexcept
	{
		return _json;
	}

	nlohmann::json &WorldDocument::editJson() noexcept
	{
		_dirty = true;
		return _json;
	}

	const VoxelGrid &WorldDocument::grid() const noexcept
	{
		return _grid;
	}

	bool WorldDocument::dirty() const noexcept
	{
		return _dirty;
	}

	void WorldDocument::markChanged() noexcept
	{
		_dirty = true;
	}

	void WorldDocument::rename(const std::string &p_id)
	{
		if (p_id == _id)
		{
			return;
		}
		if (!validId(p_id))
		{
			throw std::invalid_argument("Ids use lower-case letters, digits, and single hyphens");
		}
		_id = p_id;
		_file = _file.parent_path() / (_id + ".json");
		_dirty = true;
	}

	std::string WorldDocument::_paletteToken(const VoxelCell &p_cell)
	{
		nlohmann::json &palette = _json["palette"];
		if (!palette.is_object())
		{
			palette = nlohmann::json::object({{"0", nullptr}});
		}
		if (p_cell.isEmpty())
		{
			for (const auto &[token, value] : palette.items())
			{
				if (value.is_null()) return token;
			}
			palette["0"] = nullptr;
			return "0";
		}
		const std::string &voxelId = _voxels->stringId(p_cell.id);
		int next = 0;
		for (const auto &[token, value] : palette.items())
		{
			if (value.is_string() && value.get<std::string>() == voxelId) return token;
			try { next = std::max(next, std::stoi(token) + 1); } catch (...) {}
		}
		const std::string token = std::to_string(next);
		palette[token] = voxelId;
		return token;
	}

	void WorldDocument::_setCellOverride(const spk::Vector3Int &p_at, const VoxelCell &p_cell)
	{
		nlohmann::json &cells = _json["cells"];
		if (!cells.is_array()) cells = nlohmann::json::array();
		auto iterator = std::ranges::find_if(cells, [&p_at](const nlohmann::json &p_entry) {
			return p_entry.contains("at") && jsonVector(p_entry.at("at")) == p_at;
		});
		nlohmann::json entry = {{"at", vectorJson(p_at)}, {"voxel", _paletteToken(p_cell)}};
		if (!p_cell.isEmpty() && p_cell.orientation != VoxelOrientation::PositiveZ)
		{
			entry["orientation"] = orientationName(p_cell.orientation);
		}
		if (!p_cell.isEmpty() && p_cell.flip == VoxelFlip::NegativeY)
		{
			entry["flip"] = "negativeY";
		}
		if (iterator == cells.end()) cells.push_back(std::move(entry));
		else *iterator = std::move(entry);
	}

	bool WorldDocument::setCell(const spk::Vector3Int &p_at, const VoxelCell &p_cell)
	{
		if (!_grid.isWithinBounds(p_at)) return false;
		if (_grid.cell(p_at) == p_cell) return true;
		_grid.cell(p_at) = p_cell;
		_setCellOverride(p_at, p_cell);
		_dirty = true;
		return true;
	}

	std::vector<spk::Vector3Int> WorldDocument::boxCells(
		const spk::Vector3Int &p_first,
		const spk::Vector3Int &p_second)
	{
		const spk::Vector3Int minimum{
			std::min(p_first.x, p_second.x), std::min(p_first.y, p_second.y), std::min(p_first.z, p_second.z)};
		const spk::Vector3Int maximum{
			std::max(p_first.x, p_second.x), std::max(p_first.y, p_second.y), std::max(p_first.z, p_second.z)};
		std::vector<spk::Vector3Int> result;
		result.reserve(static_cast<std::size_t>(maximum.x - minimum.x + 1) *
			static_cast<std::size_t>(maximum.y - minimum.y + 1) *
			static_cast<std::size_t>(maximum.z - minimum.z + 1));
		for (int y = minimum.y; y <= maximum.y; ++y)
			for (int z = minimum.z; z <= maximum.z; ++z)
				for (int x = minimum.x; x <= maximum.x; ++x)
					result.push_back({x, y, z});
		return result;
	}

	void WorldDocument::fillBox(
		const spk::Vector3Int &p_first,
		const spk::Vector3Int &p_second,
		const VoxelCell &p_cell)
	{
		for (const spk::Vector3Int &cell : boxCells(p_first, p_second))
		{
			if (_grid.isWithinBounds(cell)) setCell(cell, p_cell);
		}
	}

	std::optional<spk::Vector3Int> WorldDocument::adjacentCell(const VoxelHit &p_hit)
	{
		spk::Vector3Int offset{};
		switch (p_hit.enterFace)
		{
		case VoxelAxisPlane::PositiveX: offset = {1, 0, 0}; break;
		case VoxelAxisPlane::NegativeX: offset = {-1, 0, 0}; break;
		case VoxelAxisPlane::PositiveY: offset = {0, 1, 0}; break;
		case VoxelAxisPlane::NegativeY: offset = {0, -1, 0}; break;
		case VoxelAxisPlane::PositiveZ: offset = {0, 0, 1}; break;
		case VoxelAxisPlane::NegativeZ: offset = {0, 0, -1}; break;
		case VoxelAxisPlane::Count: return std::nullopt;
		}
		return p_hit.cell + offset;
	}

	void WorldDocument::addStamp(
		const std::string &p_prefabId,
		const spk::Vector3Int &p_at,
		VoxelOrientation p_orientation,
		const PrefabDefinition &p_prefab)
	{
		for (int y = 0; y < p_prefab.size().y; ++y)
		{
			for (int z = 0; z < p_prefab.size().z; ++z)
			{
				for (int x = 0; x < p_prefab.size().x; ++x)
				{
					const spk::Vector3Int source{x, y, z};
					const spk::Vector3Int target = p_at + rotatePosition(source, p_prefab.size(), p_orientation);
					if (!_grid.isWithinBounds(target))
					{
						throw std::out_of_range("Prefab stamp extends outside the document");
					}
					VoxelCell value = p_prefab.grid.cell(source);
					if (!value.isEmpty())
					{
						value.orientation = orientationFromTurns(
							orientationTurns(value.orientation) + orientationTurns(p_orientation));
					}
					_grid.cell(target) = value;
				}
			}
		}
		_json["stamps"].push_back({
			{"prefab", p_prefabId}, {"at", vectorJson(p_at)}, {"orientation", orientationName(p_orientation)}});
		_dirty = true;
	}

	void WorldDocument::_writeDenseCells()
	{
		_json.erase("fill");
		_json["cells"] = nlohmann::json::array();
		for (int y = 0; y < _grid.size().y; ++y)
			for (int z = 0; z < _grid.size().z; ++z)
				for (int x = 0; x < _grid.size().x; ++x)
				{
					const spk::Vector3Int at{x, y, z};
					const VoxelCell &cell = _grid.cell(at);
					if (!cell.isEmpty()) _setCellOverride(at, cell);
				}
	}

	void WorldDocument::resize(const spk::Vector3Int &p_size)
	{
		if (p_size.x <= 0 || p_size.y <= 0 || p_size.z <= 0 || p_size == _grid.size()) return;
		VoxelGrid resized(p_size);
		const spk::Vector3Int overlap{
			std::min(p_size.x, _grid.size().x), std::min(p_size.y, _grid.size().y), std::min(p_size.z, _grid.size().z)};
		for (int y = 0; y < overlap.y; ++y)
			for (int z = 0; z < overlap.z; ++z)
				for (int x = 0; x < overlap.x; ++x)
					resized.cell(x, y, z) = _grid.cell(x, y, z);
		_grid = std::move(resized);
		_json["size"] = vectorJson(p_size);
		_writeDenseCells();
		_dirty = true;
	}

	void WorldDocument::save(const JsonWriter &p_writer)
	{
		p_writer.write(_file, _json);
		if (!_sourceFile.empty() && _sourceFile != _file)
		{
			std::error_code error;
			std::filesystem::remove(_sourceFile, error);
			if (error) throw std::runtime_error("Unable to remove renamed document: " + error.message());
		}
		_sourceFile = _file;
		_dirty = false;
	}

	WorldEditorPage::WorldEditorPage(
		const std::string &p_name,
		spk::Widget *p_parent,
		ToolServices &p_services) :
		IEditorPage(p_name, p_parent),
		_services(p_services),
		_background(p_name + "/Background", this),
		_leftPane(p_name + "/Left", this),
		_newMapButton(p_name + "/NewMap", "New Map", &_leftPane),
		_newPrefabButton(p_name + "/NewPrefab", "New Prefab", &_leftPane),
		_saveButton(p_name + "/Save", "Save", &_leftPane),
		_toolButton(p_name + "/Tool", "Tool: Place", &_leftPane),
		_voxelPicker(p_name + "/VoxelPalette", &_leftPane),
		_prefabPicker(p_name + "/PrefabPalette", &_leftPane),
		_viewport(p_name + "/Viewport", this),
		_properties(p_name + "/Properties", this),
		_atlas(std::make_shared<spk::SpriteSheet>())
	{
		_atlas->loadFromFile(std::filesystem::path(PG_RESOURCE_DIR) / "textures" / "voxels.png", {8u, 8u});
		_viewport.gameEngine().add<ChunkSynchronizationLogic>();
		_viewport.gameEngine().add<ChunkRenderLogic>(*_atlas);

		_background.activate();
		_leftPane.activate();
		_newMapButton.activate();
		_newPrefabButton.activate();
		_saveButton.activate();
		_toolButton.activate();
		_voxelPicker.activate();
		_prefabPicker.activate();
		_viewport.activate();
		_properties.activate();

		_newMapButton.subscribeToClick([this]() { _newDocument(WorldDocument::Kind::Map); }).relinquish();
		_newPrefabButton.subscribeToClick([this]() { _newDocument(WorldDocument::Kind::Prefab); }).relinquish();
		_saveButton.subscribeToClick([this]() { save(); }).relinquish();
		_toolButton.subscribeToClick([this]() { _cycleTool(); }).relinquish();

		_voxelPicker.setIds(_services.registries().voxels().ids());
		if (!_services.registries().voxels().ids().empty())
		{
			_voxelPicker.select(_services.registries().voxels().ids().front());
		}
		_prefabPicker.setIds(_services.registries().prefabs().ids());
		if (!_services.registries().prefabs().ids().empty())
		{
			_prefabPicker.select(_services.registries().prefabs().ids().front());
		}

		_mainLayout.setElementPadding({6, 6});
		_mainLayout.addWidget(&_leftPane, spk::Layout::SizePolicy::Fixed)->setSize({240, 0});
		_mainLayout.addWidget(&_viewport);
		_mainLayout.addWidget(&_properties, spk::Layout::SizePolicy::Fixed)->setSize({340, 0});
		_leftLayout.setElementPadding({4, 4});
		reload();
	}

	WorldEditorPage::~WorldEditorPage()
	{
		_world.reset();
	}

	void WorldEditorPage::_onGeometryChange()
	{
		_background.setGeometry(geometry().atOrigin());
		_mainLayout.setGeometry(geometry().atOrigin().shrink({6, 6}));
		_leftLayout.setGeometry(_leftPane.geometry().atOrigin());
	}

	void WorldEditorPage::_onMouseMovedEvent(spk::MouseMovedEvent &p_event)
	{
		if (_viewport.absoluteGeometry().contains(p_event.device().position) &&
			p_event.device()[spk::Mouse::Middle] == spk::InputState::Down)
		{
			_editCamera.pan(_viewport, p_event->delta);
			p_event.consume();
		}
	}

	void WorldEditorPage::_onMouseButtonPressedEvent(spk::MouseButtonPressedEvent &p_event)
	{
		if (p_event->button != spk::Mouse::Left || _world == nullptr ||
			!_viewport.absoluteGeometry().contains(p_event.device().position))
		{
			return;
		}
		const spk::Rect2D &rect = _viewport.absoluteGeometry();
		const spk::Vector2 local{
			static_cast<float>(p_event.device().position.x - rect.anchor.x),
			static_cast<float>(p_event.device().position.y - rect.anchor.y)};
		const WorldRay ray = MousePicker::screenToRay(
			_viewport.camera(),
			{static_cast<float>(rect.width()), static_cast<float>(rect.height())},
			local);
		_applyTool(WorldRaycaster::raycast(*_world, ray, 1000.0f), ray);
		p_event.consume();
	}

	void WorldEditorPage::_onKeyPressedEvent(spk::KeyPressedEvent &p_event)
	{
		if (p_event->key == spk::Keyboard::R)
		{
			_editCamera.rotateClockwise();
			_services.report("Brush orientation: " + orientationName(_editCamera.orientation()));
			p_event.consume();
		}
		else if (p_event->key == spk::Keyboard::F)
		{
			_editCamera.toggleFlip();
			_services.report(_editCamera.flip() == VoxelFlip::PositiveY ? "Brush flip: positiveY" : "Brush flip: negativeY");
			p_event.consume();
		}
	}

	void WorldEditorPage::_rebuildDocumentList()
	{
		_leftLayout.clear();
		_documentButtons.clear();
		_leftLayout.addWidget(&_newMapButton, spk::Layout::SizePolicy::Minimum);
		_leftLayout.addWidget(&_newPrefabButton, spk::Layout::SizePolicy::Minimum);
		_leftLayout.addWidget(&_saveButton, spk::Layout::SizePolicy::Minimum);
		_leftLayout.addWidget(&_toolButton, spk::Layout::SizePolicy::Minimum);
		_leftLayout.addWidget(&_voxelPicker, spk::Layout::SizePolicy::Minimum);
		_leftLayout.addWidget(&_prefabPicker, spk::Layout::SizePolicy::Minimum);

		const auto addDirectory = [this](const std::filesystem::path &p_directory, WorldDocument::Kind p_kind) {
			std::vector<std::filesystem::path> files;
			for (const auto &entry : std::filesystem::directory_iterator(p_directory))
			{
				if (entry.is_regular_file() && entry.path().extension() == ".json") files.push_back(entry.path());
			}
			std::ranges::sort(files);
			for (const auto &file : files)
			{
				const std::string prefix = p_kind == WorldDocument::Kind::Map ? "Map: " : "Prefab: ";
				auto button = std::make_unique<spk::PushButton>(
					name() + "/Document/" + file.stem().string(), prefix + file.stem().string(), &_leftPane);
				button->subscribeToClick([this, file, p_kind]() { _selectDocument(file, p_kind); }).relinquish();
				button->activate();
				_leftLayout.addWidget(button.get(), spk::Layout::SizePolicy::Minimum);
				_documentButtons.push_back(std::move(button));
			}
		};
		addDirectory(_services.dataDirectory() / "maps", WorldDocument::Kind::Map);
		addDirectory(_services.dataDirectory() / "prefabs", WorldDocument::Kind::Prefab);

		const auto addElements = [this](
			const char *p_array,
			const char *p_caption,
			SelectionKind p_selection,
			const char *p_nameField) {
			if (!_document.json().contains(p_array) || !_document.json().at(p_array).is_array()) return;
			const nlohmann::json &values = _document.json().at(p_array);
			for (std::size_t index = 0; index < values.size(); ++index)
			{
				const std::string label = values[index].value(p_nameField, std::string("#") + std::to_string(index + 1));
				auto button = std::make_unique<spk::PushButton>(
					name() + "/Element/" + p_array + "/" + std::to_string(index),
					std::string(p_caption) + ": " + label,
					&_leftPane);
				button->subscribeToClick([this, p_selection, index]() {
					_selection = p_selection;
					_selectionIndex = index;
					_rebuildProperties();
				}).relinquish();
				button->activate();
				_leftLayout.addWidget(button.get(), spk::Layout::SizePolicy::Minimum);
				_documentButtons.push_back(std::move(button));
			}
		};
		if (_document.kind() == WorldDocument::Kind::Map)
		{
			addElements("markers", "Marker", SelectionKind::Marker, "name");
			addElements("portals", "Portal", SelectionKind::Portal, "name");
			addElements("trainers", "Trainer", SelectionKind::Trainer, "id");
			addElements("stamps", "Stamp", SelectionKind::Stamp, "prefab");
		}
		else
		{
			addElements("anchors", "Anchor", SelectionKind::Anchor, "name");
		}
		_onGeometryChange();
	}

	void WorldEditorPage::_rebuildWorld()
	{
		_world.reset();
		_viewport.clearScene();
		MapDefinition preview{.id = _document.id(), .grid = _document.grid()};
		_world = std::make_unique<VoxelWorld>(_services.registries().voxels(), &_viewport.gameEngine());
		_world->loadFromMap(preview);
		const spk::Vector3Int size = _document.grid().size();
		_viewport.frame(
			{static_cast<float>(size.x) * 0.5f, static_cast<float>(size.y) * 0.25f, static_cast<float>(size.z) * 0.5f},
			std::max({8.0f, static_cast<float>(size.x), static_cast<float>(size.z)}) * 0.9f);
		_viewport.invalidateRenderUnit();
	}

	void WorldEditorPage::_selectDocument(const std::filesystem::path &p_file, WorldDocument::Kind p_kind)
	{
		if (_document.dirty())
		{
			_services.report("Save or reload the current document before switching");
			return;
		}
		_document.load(p_file, p_kind, _services.registries());
		_selection = SelectionKind::None;
		_boxStart.reset();
		_state.markSaved();
		_rebuildWorld();
		_rebuildProperties();
		_services.report("Loaded " + _document.id());
	}

	void WorldEditorPage::_newDocument(WorldDocument::Kind p_kind)
	{
		if (_document.dirty())
		{
			_services.report("Save or reload the current document before creating another");
			return;
		}
		const std::filesystem::path directory = _services.dataDirectory() /
			(p_kind == WorldDocument::Kind::Map ? "maps" : "prefabs");
		const std::string base = p_kind == WorldDocument::Kind::Map ? "new-map" : "new-prefab";
		std::string id = base;
		for (int suffix = 2; std::filesystem::exists(directory / (id + ".json")); ++suffix)
		{
			id = base + "-" + std::to_string(suffix);
		}
		_document.create(
			directory,
			id,
			p_kind,
			_services.registries().voxels(),
			p_kind == WorldDocument::Kind::Map ? spk::Vector3Int{32, 8, 32} : spk::Vector3Int{8, 8, 8});
		_selection = SelectionKind::None;
		_state.markChanged();
		_rebuildWorld();
		_rebuildProperties();
		_services.report("Created " + id + "; edit its Id before saving if needed");
	}

	void WorldEditorPage::_cycleTool()
	{
		static constexpr std::array names = {
			"Place", "Erase", "Box Fill", "Marker", "Portal", "Trainer", "Stamp", "Anchor"};
		_tool = static_cast<ToolMode>((static_cast<int>(_tool) + 1) % static_cast<int>(names.size()));
		_toolButton.setText("Tool: " + std::string(names[static_cast<std::size_t>(_tool)]));
		_boxStart.reset();
		_services.report("Tool: " + std::string(names[static_cast<std::size_t>(_tool)]) + " (R rotate, F flip, middle-drag pan)");
	}

	VoxelCell WorldEditorPage::_brushCell() const
	{
		if (_voxelPicker.selected().empty()) return {};
		return {
			_services.registries().voxels().numericId(_voxelPicker.selected()),
			_editCamera.orientation(),
			_editCamera.flip()};
	}

	std::optional<spk::Vector3Int> WorldEditorPage::_groundCell(const WorldRay &p_ray) const
	{
		if (std::abs(p_ray.direction.y) < 0.0001f) return std::nullopt;
		const float distance = -p_ray.origin.y / p_ray.direction.y;
		if (distance < 0.0f) return std::nullopt;
		const spk::Vector3 point = p_ray.origin + p_ray.direction * distance;
		const spk::Vector3Int cell{static_cast<int>(std::floor(point.x)), 0, static_cast<int>(std::floor(point.z))};
		return _document.grid().isWithinBounds(cell) ? std::optional(cell) : std::nullopt;
	}

	void WorldEditorPage::_applyTool(const std::optional<VoxelHit> &p_hit, const WorldRay &p_ray)
	{
		const std::optional<spk::Vector3Int> adjacent = p_hit.has_value()
			? WorldDocument::adjacentCell(*p_hit)
			: _groundCell(p_ray);
		if (_tool == ToolMode::Erase)
		{
			if (p_hit.has_value() && _document.setCell(p_hit->cell, {})) _changed();
			return;
		}
		if (!adjacent.has_value() || !_document.grid().isWithinBounds(*adjacent))
		{
			_services.report("No editable cell under the cursor");
			return;
		}
		if (_tool == ToolMode::Place)
		{
			if (_document.setCell(*adjacent, _brushCell())) _changed();
		}
		else if (_tool == ToolMode::BoxFill)
		{
			if (!_boxStart.has_value())
			{
				_boxStart = *adjacent;
				_services.report("Box start selected; click the opposite corner");
			}
			else
			{
				_document.fillBox(*_boxStart, *adjacent, _brushCell());
				_boxStart.reset();
				_changed();
			}
		}
		else
		{
			_addPlacedElement(*adjacent);
		}
	}

	void WorldEditorPage::_addPlacedElement(const spk::Vector3Int &p_at)
	{
		nlohmann::json &root = _document.editJson();
		if (_tool == ToolMode::Marker && _document.kind() == WorldDocument::Kind::Map)
		{
			auto &values = root["markers"];
			values.push_back({{"name", "marker-" + std::to_string(values.size() + 1)}, {"at", vectorJson(p_at)}});
			_selection = SelectionKind::Marker;
			_selectionIndex = values.size() - 1;
		}
		else if (_tool == ToolMode::Portal && _document.kind() == WorldDocument::Kind::Map)
		{
			auto &values = root["portals"];
			values.push_back({
				{"name", "portal-" + std::to_string(values.size() + 1)},
				{"at", vectorJson(p_at)},
				{"target", {{"map", _document.id()}, {"portal", "portal"}}}});
			_selection = SelectionKind::Portal;
			_selectionIndex = values.size() - 1;
		}
		else if (_tool == ToolMode::Trainer && _document.kind() == WorldDocument::Kind::Map)
		{
			auto &values = root["trainers"];
			const std::vector<std::string> encounters = _services.registries().encounterTables().ids();
			const std::string encounter = encounters.empty() ? "encounter" : encounters.front();
			const std::string id = "trainer-" + std::to_string(values.size() + 1);
			values.push_back({
				{"id", id}, {"at", vectorJson(p_at)}, {"facing", orientationName(_editCamera.orientation())},
				{"sightRange", 6}, {"encounterTable", encounter}, {"clearedFlag", id}, {"boardSize", {11, 11}}});
			_selection = SelectionKind::Trainer;
			_selectionIndex = values.size() - 1;
		}
		else if (_tool == ToolMode::Stamp && _document.kind() == WorldDocument::Kind::Map)
		{
			if (_prefabPicker.selected().empty()) return;
			_document.addStamp(
				_prefabPicker.selected(), p_at, _editCamera.orientation(),
				_services.registries().prefabs().get(_prefabPicker.selected()));
			_selection = SelectionKind::Stamp;
			_selectionIndex = _document.json().at("stamps").size() - 1;
		}
		else if (_tool == ToolMode::Anchor && _document.kind() == WorldDocument::Kind::Prefab)
		{
			auto &values = root["anchors"];
			values.push_back({{"name", "anchor-" + std::to_string(values.size() + 1)}, {"at", vectorJson(p_at)}});
			_selection = SelectionKind::Anchor;
			_selectionIndex = values.size() - 1;
		}
		else
		{
			_services.report("That tool is not available for this document type");
			return;
		}
		_changed();
		_rebuildDocumentList();
		_rebuildProperties();
	}

	void WorldEditorPage::_changed(bool p_rebuildWorld)
	{
		_document.markChanged();
		_state.markChanged();
		if (p_rebuildWorld) _rebuildWorld();
	}

	void WorldEditorPage::_rebuildProperties()
	{
		_properties.clear();
		if (_document.id().empty()) return;

		spk::TextEdit &idEdit = _properties.addString("Id", _document.id(), [this](const std::string &p_value) {
			if (!validId(p_value)) return;
			_document.rename(p_value);
			_state.markChanged();
		});
		idEdit.setValidationCallback([idEdit = &idEdit](const spk::Font::Text &) {
			return validId(idEdit->textAsUTF8())
				? spk::TextEdit::ValidationState::Valid
				: spk::TextEdit::ValidationState::Invalid;
		});
		_properties.addString(
			"Kind", _document.kind() == WorldDocument::Kind::Map ? "Map" : "Prefab", [](const std::string &) {}).disableEdit();

		const auto resizeAxis = [this](int p_axis, int p_value) {
			spk::Vector3Int size = _document.grid().size();
			if (p_axis == 0) size.x = p_value;
			else if (p_axis == 1) size.y = p_value;
			else size.z = p_value;
			_document.resize(size);
			_changed();
		};
		const spk::Vector3Int size = _document.grid().size();
		_properties.addInt("Size X", size.x, 1, 512, [resizeAxis](int p_value) { resizeAxis(0, p_value); });
		_properties.addInt("Size Y", size.y, 1, 128, [resizeAxis](int p_value) { resizeAxis(1, p_value); });
		_properties.addInt("Size Z", size.z, 1, 512, [resizeAxis](int p_value) { resizeAxis(2, p_value); });

		if (_document.kind() == WorldDocument::Kind::Map)
		{
			auto biome = std::make_unique<IdPicker>(name() + "/Biome", &_properties);
			biome->setIds(_services.registries().biomes().ids());
			biome->select(_document.json().value("biome", ""));
			biome->subscribeToSelection([this](const std::string &p_id) {
				_document.editJson()["biome"] = p_id;
				_changed(false);
			});
			_properties.addCustom("Biome", std::move(biome));
		}

		std::string arrayName;
		switch (_selection)
		{
		case SelectionKind::Marker: arrayName = "markers"; break;
		case SelectionKind::Portal: arrayName = "portals"; break;
		case SelectionKind::Trainer: arrayName = "trainers"; break;
		case SelectionKind::Stamp: arrayName = "stamps"; break;
		case SelectionKind::Anchor: arrayName = "anchors"; break;
		case SelectionKind::None: return;
		}
		if (!_document.json().contains(arrayName) || _selectionIndex >= _document.json().at(arrayName).size()) return;
		const nlohmann::json &selected = _document.json().at(arrayName).at(_selectionIndex);

		if (selected.contains("name"))
		{
			_properties.addString("Name", selected.at("name").get<std::string>(), [this, arrayName](const std::string &p_value) {
				_document.editJson()[arrayName][_selectionIndex]["name"] = p_value;
				_changed(false);
			});
		}
		if (selected.contains("id"))
		{
			_properties.addString("Trainer Id", selected.at("id").get<std::string>(), [this, arrayName](const std::string &p_value) {
				_document.editJson()[arrayName][_selectionIndex]["id"] = p_value;
				_changed(false);
			});
		}
		if (selected.contains("prefab"))
		{
			_properties.addString("Prefab", selected.at("prefab").get<std::string>(), [](const std::string &) {}).disableEdit();
		}

		if (selected.contains("at"))
		{
			const spk::Vector3Int at = jsonVector(selected.at("at"));
			const auto editCoordinate = [this, arrayName](int p_axis, int p_value) {
				nlohmann::json &atValue = _document.editJson()[arrayName][_selectionIndex]["at"];
				atValue[static_cast<std::size_t>(p_axis)] = p_value;
				_changed(false);
			};
			_properties.addInt("At X", at.x, 0, std::max(0, size.x - 1), [editCoordinate](int p_value) { editCoordinate(0, p_value); });
			_properties.addInt("At Y", at.y, 0, std::max(0, size.y - 1), [editCoordinate](int p_value) { editCoordinate(1, p_value); });
			_properties.addInt("At Z", at.z, 0, std::max(0, size.z - 1), [editCoordinate](int p_value) { editCoordinate(2, p_value); });
		}

		if (_selection == SelectionKind::Portal)
		{
			auto mapPicker = std::make_unique<IdPicker>(name() + "/PortalMap", &_properties);
			std::vector<std::string> ids = _services.registries().maps().ids();
			if (!std::ranges::contains(ids, _document.id())) ids.push_back(_document.id());
			mapPicker->setIds(std::move(ids));
			mapPicker->select(selected.at("target").value("map", ""));
			mapPicker->subscribeToSelection([this](const std::string &p_id) {
				_document.editJson()["portals"][_selectionIndex]["target"]["map"] = p_id;
				_changed(false);
			});
			_properties.addCustom("Target Map", std::move(mapPicker));
			_properties.addString(
				"Target Portal", selected.at("target").value("portal", ""), [this](const std::string &p_value) {
					_document.editJson()["portals"][_selectionIndex]["target"]["portal"] = p_value;
					_changed(false);
				});
		}
		else if (_selection == SelectionKind::Trainer)
		{
			_properties.addEnum(
				"Facing", {"positiveZ", "positiveX", "negativeZ", "negativeX"}, selected.value("facing", "positiveZ"),
				[this](const std::string &p_value) {
					_document.editJson()["trainers"][_selectionIndex]["facing"] = p_value;
					_changed(false);
				});
			_properties.addInt("Sight Range", selected.value("sightRange", 6), 1, 64, [this](int p_value) {
				_document.editJson()["trainers"][_selectionIndex]["sightRange"] = p_value;
				_changed(false);
			});
			auto encounter = std::make_unique<IdPicker>(name() + "/TrainerEncounter", &_properties);
			encounter->setIds(_services.registries().encounterTables().ids());
			encounter->select(selected.value("encounterTable", ""));
			encounter->subscribeToSelection([this](const std::string &p_id) {
				_document.editJson()["trainers"][_selectionIndex]["encounterTable"] = p_id;
				_changed(false);
			});
			_properties.addCustom("Encounter", std::move(encounter));
			_properties.addString("Cleared Flag", selected.value("clearedFlag", ""), [this](const std::string &p_value) {
				_document.editJson()["trainers"][_selectionIndex]["clearedFlag"] = p_value;
				_changed(false);
			});
			const nlohmann::json board = selected.value("boardSize", nlohmann::json::array({11, 11}));
			_properties.addInt("Board X", board.at(0).get<int>(), 1, 64, [this](int p_value) {
				_document.editJson()["trainers"][_selectionIndex]["boardSize"][0] = p_value;
				_changed(false);
			});
			_properties.addInt("Board Y", board.at(1).get<int>(), 1, 64, [this](int p_value) {
				_document.editJson()["trainers"][_selectionIndex]["boardSize"][1] = p_value;
				_changed(false);
			});
		}
	}

	std::string WorldEditorPage::title() const
	{
		return "World / Structure Editor";
	}

	bool WorldEditorPage::hasUnsavedChanges() const
	{
		return _document.dirty();
	}

	void WorldEditorPage::save()
	{
		if (_document.id().empty()) return;
		_document.save(_services.writer());
		const std::filesystem::path file = _document.file();
		const WorldDocument::Kind kind = _document.kind();
		_world.reset();
		std::string reloadWarning;
		try
		{
			_services.load();
		}
		catch (const std::exception &exception)
		{
			reloadWarning = exception.what();
		}
		_document.load(file, kind, _services.registries());
		_voxelPicker.setIds(_services.registries().voxels().ids());
		_prefabPicker.setIds(_services.registries().prefabs().ids());
		_state.markSaved();
		_rebuildDocumentList();
		_rebuildWorld();
		_rebuildProperties();
		if (reloadWarning.empty())
			_services.report("Saved " + file.filename().string() + " and reloaded registries");
		else
			_services.report("Saved, but full registry reload is waiting on linked content: " + reloadWarning);
	}

	void WorldEditorPage::reload()
	{
		std::filesystem::path file = _document.file();
		WorldDocument::Kind kind = _document.kind();
		if (file.empty() || !std::filesystem::exists(file))
		{
			file = _services.dataDirectory() / "maps" / "m1-testground.json";
			kind = WorldDocument::Kind::Map;
			if (!std::filesystem::exists(file))
			{
				for (const auto &entry : std::filesystem::directory_iterator(_services.dataDirectory() / "maps"))
				{
					if (entry.path().extension() == ".json") { file = entry.path(); break; }
				}
			}
		}
		_document.load(file, kind, _services.registries());
		_selection = SelectionKind::None;
		_boxStart.reset();
		_state.markSaved();
		_rebuildDocumentList();
		_rebuildWorld();
		_rebuildProperties();
		_services.report("World editor loaded " + _document.id());
	}
}
