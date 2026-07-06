#include "tools/pages/voxel_modeler_page.hpp"

#include "core/json.hpp"
#include "structures/game_engine/spk_texture_mesh_renderer_3d.hpp"
#include "structures/graphics/geometry/spk_primitive_object.hpp"
#include "tools/widgets/atlas_cell_picker.hpp"
#include "voxel/voxel_grid.hpp"
#include "voxel/voxel_mesher.hpp"
#include "voxel/voxel_registry.hpp"

#include <algorithm>
#include <array>
#include <chrono>
#include <cctype>
#include <sstream>
#include <stdexcept>
#include <utility>

namespace
{
	[[nodiscard]] std::string trim(std::string p_value)
	{
		const auto first = std::ranges::find_if(p_value, [](unsigned char p_character) { return !std::isspace(p_character); });
		const auto last = std::ranges::find_if(p_value.rbegin(), p_value.rend(), [](unsigned char p_character) { return !std::isspace(p_character); }).base();
		return first < last ? std::string(first, last) : std::string{};
	}

	[[nodiscard]] std::vector<std::string> splitTags(const std::string &p_text)
	{
		std::vector<std::string> result;
		std::stringstream stream(p_text);
		std::string tag;
		while (std::getline(stream, tag, ','))
		{
			tag = trim(std::move(tag));
			if (!tag.empty())
			{
				result.push_back(std::move(tag));
			}
		}
		return result;
	}

	[[nodiscard]] bool isValidId(const std::string &p_id)
	{
		if (p_id.empty() || p_id.front() == '-' || p_id.back() == '-')
		{
			return false;
		}
		bool previousDash = false;
		for (const unsigned char character : p_id)
		{
			const bool dash = character == '-';
			if ((!std::islower(character) && !std::isdigit(character) && !dash) || (dash && previousDash))
			{
				return false;
			}
			previousDash = dash;
		}
		return true;
	}

	[[nodiscard]] std::string joinTags(const nlohmann::json &p_tags)
	{
		std::string result;
		if (!p_tags.is_array())
		{
			return result;
		}
		for (const nlohmann::json &tag : p_tags)
		{
			if (!tag.is_string())
			{
				continue;
			}
			if (!result.empty())
			{
				result += ", ";
			}
			result += tag.get<std::string>();
		}
		return result;
	}
}

namespace pg::tools
{
	void VoxelModelDocument::load(const std::filesystem::path &p_directory)
	{
		_directory = p_directory;
		_definitions.clear();
		_dirty.clear();
		_deleted.clear();
		_selected.clear();
		std::vector<std::filesystem::path> files;
		for (const auto &entry : std::filesystem::directory_iterator(_directory))
		{
			if (entry.is_regular_file() && entry.path().extension() == ".json")
			{
				files.push_back(entry.path());
			}
		}
		std::ranges::sort(files);
		for (const auto &file : files)
		{
			_definitions.emplace(file.stem().string(), JsonLoader::parseFile(file));
		}
		if (!_definitions.empty())
		{
			_selected = _definitions.begin()->first;
		}
	}

	std::vector<std::string> VoxelModelDocument::ids() const
	{
		std::vector<std::string> result;
		result.reserve(_definitions.size());
		for (const auto &[id, unused] : _definitions)
		{
			(void)unused;
			result.push_back(id);
		}
		return result;
	}

	bool VoxelModelDocument::contains(const std::string &p_id) const
	{
		return _definitions.contains(p_id);
	}

	void VoxelModelDocument::select(const std::string &p_id)
	{
		if (!contains(p_id))
		{
			throw std::out_of_range("Unknown voxel definition '" + p_id + "'");
		}
		_selected = p_id;
	}

	const std::string &VoxelModelDocument::selectedId() const noexcept
	{
		return _selected;
	}

	nlohmann::json &VoxelModelDocument::selected()
	{
		return _definitions.at(_selected);
	}

	const nlohmann::json &VoxelModelDocument::selected() const
	{
		return _definitions.at(_selected);
	}

	std::string VoxelModelDocument::create(std::string p_baseId, nlohmann::json p_definition)
	{
		p_baseId = trim(std::move(p_baseId));
		if (p_baseId.empty())
		{
			p_baseId = "new-voxel";
		}
		std::string id = p_baseId;
		for (std::size_t suffix = 2; _definitions.contains(id); ++suffix)
		{
			id = p_baseId + "-" + std::to_string(suffix);
		}
		_definitions.emplace(id, std::move(p_definition));
		_dirty.insert(id);
		_selected = id;
		return id;
	}

	void VoxelModelDocument::renameSelected(const std::string &p_id)
	{
		if (p_id == _selected)
		{
			return;
		}
		if (!isValidId(p_id))
		{
			throw std::invalid_argument("Voxel ids use lower-case letters, digits, and single hyphens");
		}
		if (_definitions.contains(p_id))
		{
			throw std::invalid_argument("Voxel id '" + p_id + "' already exists");
		}
		const std::string oldId = _selected;
		auto node = _definitions.extract(oldId);
		node.key() = p_id;
		_definitions.insert(std::move(node));
		_dirty.erase(oldId);
		_dirty.insert(p_id);
		_deleted.insert(oldId);
		_deleted.erase(p_id);
		_selected = p_id;
	}

	std::string VoxelModelDocument::duplicateSelected()
	{
		if (_selected.empty())
		{
			throw std::logic_error("No voxel is selected");
		}
		return create(_selected + "-copy", selected());
	}

	void VoxelModelDocument::deleteSelected()
	{
		if (_selected.empty())
		{
			return;
		}
		const std::string id = _selected;
		const auto next = std::next(_definitions.find(id));
		_definitions.erase(id);
		_dirty.erase(id);
		_deleted.insert(id);
		if (_definitions.empty())
		{
			_selected.clear();
		}
		else
		{
			_selected = next == _definitions.end() ? _definitions.rbegin()->first : next->first;
		}
	}

	void VoxelModelDocument::markSelectedChanged()
	{
		if (!_selected.empty())
		{
			_dirty.insert(_selected);
		}
	}

	void VoxelModelDocument::save(const JsonWriter &p_writer)
	{
		for (const std::string &id : _dirty)
		{
			p_writer.write(_directory / (id + ".json"), _definitions.at(id));
		}
		for (const std::string &id : _deleted)
		{
			std::error_code error;
			std::filesystem::remove(_directory / (id + ".json"), error);
			if (error)
			{
				throw std::runtime_error("Unable to delete voxel definition '" + id + "': " + error.message());
			}
		}
		_dirty.clear();
		_deleted.clear();
	}

	bool VoxelModelDocument::dirty() const noexcept
	{
		return !_dirty.empty() || !_deleted.empty();
	}

	VoxelModelerPage::VoxelModelerPage(
		const std::string &p_name,
		spk::Widget *p_parent,
		ToolServices &p_services) :
		IEditorPage(p_name, p_parent),
		_services(p_services),
		_background(p_name + "/Background", this),
		_listPane(p_name + "/Definitions", this),
		_newButton(p_name + "/New", "New", &_listPane),
		_duplicateButton(p_name + "/Duplicate", "Duplicate", &_listPane),
		_deleteButton(p_name + "/Delete", "Delete", &_listPane),
		_saveButton(p_name + "/Save", "Save", &_listPane),
		_previewModeButton(p_name + "/PreviewMode", "Preview: orientations", &_listPane),
		_viewport(p_name + "/Viewport", this),
		_properties(p_name + "/Properties", this),
		_atlas(std::make_shared<spk::SpriteSheet>())
	{
		_atlas->loadFromFile(std::filesystem::path(PG_RESOURCE_DIR) / "textures" / "voxels.png", {8u, 8u});
		const auto unique = std::chrono::steady_clock::now().time_since_epoch().count();
		_previewDirectory = std::filesystem::temp_directory_path() / ("erelia-voxel-preview-" + std::to_string(unique));
		std::filesystem::create_directories(_previewDirectory);

		_background.activate();
		_listPane.activate();
		_newButton.activate();
		_duplicateButton.activate();
		_deleteButton.activate();
		_saveButton.activate();
		_previewModeButton.activate();
		_viewport.activate();
		_properties.activate();

		_newButton.subscribeToClick([this]() {
			_document.create("new-voxel", _newVoxel());
			_rebuildList();
			_rebuildProperties();
			_rebuildPreview();
		}).relinquish();
		_duplicateButton.subscribeToClick([this]() {
			_document.duplicateSelected();
			_rebuildList();
			_rebuildProperties();
			_rebuildPreview();
		}).relinquish();
		_deleteButton.subscribeToClick([this]() {
			_document.deleteSelected();
			_rebuildList();
			_rebuildProperties();
			_rebuildPreview();
		}).relinquish();
		_saveButton.subscribeToClick([this]() { save(); }).relinquish();
		_previewModeButton.subscribeToClick([this]() {
			_contextPreview = !_contextPreview;
			_previewModeButton.setText(_contextPreview ? "Preview: neighbours" : "Preview: orientations");
			_rebuildPreview();
		}).relinquish();

		_mainLayout.setElementPadding({6, 6});
		_mainLayout.addWidget(&_listPane, spk::Layout::SizePolicy::Fixed)->setSize({220, 0});
		_mainLayout.addWidget(&_viewport);
		_mainLayout.addWidget(&_properties, spk::Layout::SizePolicy::Fixed)->setSize({330, 0});
		_listLayout.setElementPadding({4, 4});
		reload();
	}

	VoxelModelerPage::~VoxelModelerPage()
	{
		std::error_code error;
		std::filesystem::remove_all(_previewDirectory, error);
	}

	void VoxelModelerPage::_onGeometryChange()
	{
		_background.setGeometry(geometry().atOrigin());
		_mainLayout.setGeometry(geometry().atOrigin().shrink({6, 6}));
		_listLayout.setGeometry(_listPane.geometry().atOrigin());
	}

	void VoxelModelerPage::_rebuildList()
	{
		_listLayout.clear();
		_definitionButtons.clear();
		_listLayout.addWidget(&_newButton, spk::Layout::SizePolicy::Minimum);
		_listLayout.addWidget(&_duplicateButton, spk::Layout::SizePolicy::Minimum);
		_listLayout.addWidget(&_deleteButton, spk::Layout::SizePolicy::Minimum);
		_listLayout.addWidget(&_saveButton, spk::Layout::SizePolicy::Minimum);
		_listLayout.addWidget(&_previewModeButton, spk::Layout::SizePolicy::Minimum);
		for (const std::string &id : _document.ids())
		{
			auto button = std::make_unique<spk::PushButton>(name() + "/Definition/" + id, id, &_listPane);
			button->subscribeToClick([this, id]() { _select(id); }).relinquish();
			button->activate();
			_listLayout.addWidget(button.get(), spk::Layout::SizePolicy::Minimum);
			_definitionButtons.push_back(std::move(button));
		}
		_onGeometryChange();
	}

	void VoxelModelerPage::_rebuildProperties()
	{
		_properties.clear();
		if (_document.selectedId().empty())
		{
			return;
		}
		nlohmann::json &definition = _document.selected();
		spk::TextEdit &idEdit = _properties.addString("Id", _document.selectedId(), [this](const std::string &p_value) {
			if (!isValidId(p_value) || (_document.contains(p_value) && p_value != _document.selectedId()))
			{
				return;
			}
			_document.renameSelected(p_value);
			_state.markChanged();
			_rebuildList();
		});
		idEdit.setValidationCallback([this, idEdit = &idEdit](const spk::Font::Text &) {
			const std::string value = idEdit->textAsUTF8();
			return isValidId(value) && (!_document.contains(value) || value == _document.selectedId())
				? spk::TextEdit::ValidationState::Valid
				: spk::TextEdit::ValidationState::Invalid;
		});
		_properties.addEnum(
			"Traversal", {"solid", "passable"}, definition.value("traversal", "solid"),
			[this](const std::string &p_value) {
				_document.selected()["traversal"] = p_value;
				_changed();
			});
		_properties.addString("Tags", joinTags(definition.value("tags", nlohmann::json::array())), [this](const std::string &p_value) {
			_document.selected()["tags"] = splitTags(p_value);
			_changed();
		});

		nlohmann::json &shape = definition["shape"];
		const std::string type = shape.value("type", "cube");
		_properties.addEnum(
			"Shape", {"cube", "slab", "slope", "stair", "crossPlane"}, type,
			[this](const std::string &p_type) {
				nlohmann::json &current = _document.selected()["shape"];
				current = PropertyPanel::swapPolymorphicType(current, p_type, _shapeDefaults(p_type));
				_changed(true);
			});
		if (type == "slab")
		{
			_properties.addFloat("Height", shape.value("height", 0.5f), 0.0f, 1.0f, [this](float p_value) {
				_document.selected()["shape"]["height"] = p_value;
				_changed();
			});
		}
		else if (type == "stair")
		{
			_properties.addInt("Steps", shape.value("stepCount", 2), 1, 64, [this](int p_value) {
				_document.selected()["shape"]["stepCount"] = p_value;
				_changed();
			});
		}

		if (!shape.contains("textures") || !shape["textures"].is_object())
		{
			shape["textures"] = nlohmann::json::object();
		}
		std::vector<std::string> slots;
		for (const auto &[slot, unused] : shape["textures"].items())
		{
			(void)unused;
			slots.push_back(slot);
		}
		for (const std::string &slot : slots)
		{
			const nlohmann::json &value = shape["textures"][slot];
			AtlasCell cell{value.at(0).get<int>(), value.at(1).get<int>()};
			auto picker = std::make_unique<AtlasCellPicker>(name() + "/Texture/" + slot, _atlas, &_properties);
			picker->setCell(cell);
			picker->subscribeToSelection([this, slot](const AtlasCell &p_cell) {
				_document.selected()["shape"]["textures"][slot] = {p_cell.column, p_cell.row};
				_changed();
			});
			_properties.addCustom("Texture " + slot, std::move(picker));
		}
	}

	void VoxelModelerPage::_rebuildPreview()
	{
		_viewport.clearScene();
		if (_document.selectedId().empty())
		{
			return;
		}
		try
		{
			std::error_code error;
			for (const auto &entry : std::filesystem::directory_iterator(_previewDirectory))
			{
				std::filesystem::remove(entry.path(), error);
				error.clear();
			}
			_services.writer().write(_previewDirectory / (_document.selectedId() + ".json"), _document.selected());
			VoxelRegistry registry;
			registry.load(_previewDirectory);
			VoxelGrid grid(_contextPreview ? spk::Vector3Int{3, 1, 3} : spk::Vector3Int{4, 1, 1});
			if (_contextPreview)
			{
				for (int z = 0; z < 3; ++z)
				{
					for (int x = 0; x < 3; ++x)
					{
						grid.cell(x, 0, z).id = 0;
					}
				}
			}
			else
			{
				const std::array orientations = {
					VoxelOrientation::PositiveZ, VoxelOrientation::PositiveX,
					VoxelOrientation::NegativeZ, VoxelOrientation::NegativeX};
				for (int x = 0; x < 4; ++x)
				{
					grid.cell(x, 0, 0) = VoxelCell{0, orientations[static_cast<std::size_t>(x)], VoxelFlip::PositiveY};
				}
			}

			auto mesh = std::make_shared<spk::TextureMesh3D>(VoxelMesher::buildRenderMesh(grid, registry));
			spk::Entity3D &meshEntity = _viewport.createEntity();
			auto &renderer = meshEntity.addComponent<spk::TextureMeshRenderer3D>();
			renderer.setMesh(std::move(mesh));
			renderer.setTexture(_atlas.get());
			renderer.setTint({1.0f, 1.0f, 1.0f, 1.0f});

			const CardinalHeightSet &heights = registry.get(0).shape->heights(VoxelFlip::PositiveY);
			const std::array positions = {
				spk::Vector3{1.0f, heights.positiveX, 0.5f},
				spk::Vector3{0.0f, heights.negativeX, 0.5f},
				spk::Vector3{0.5f, heights.positiveZ, 1.0f},
				spk::Vector3{0.5f, heights.negativeZ, 0.0f},
				spk::Vector3{0.5f, heights.stationary, 0.5f}};
			const std::array colors = {
				spk::Color{1.0f, 0.2f, 0.2f, 1.0f}, spk::Color{0.2f, 1.0f, 0.2f, 1.0f},
				spk::Color{0.2f, 0.5f, 1.0f, 1.0f}, spk::Color{1.0f, 0.8f, 0.2f, 1.0f},
				spk::Color{1.0f, 0.2f, 1.0f, 1.0f}};
			auto postMesh = std::make_shared<spk::TextureMesh3D>(spk::PrimitiveObject::CreateCube(1.0f));
			const spk::Vector3 base = _contextPreview ? spk::Vector3{1.0f, 0.0f, 1.0f} : spk::Vector3{};
			for (std::size_t index = 0; index < positions.size(); ++index)
			{
				spk::Entity3D &post = _viewport.createEntity();
				const float height = std::max(0.02f, positions[index].y);
				post.transform().setPosition(base + spk::Vector3{positions[index].x, height * 0.5f, positions[index].z});
				post.transform().setScale({0.04f, height, 0.04f});
				auto &postRenderer = post.addComponent<spk::TextureMeshRenderer3D>();
				postRenderer.setMesh(postMesh);
				postRenderer.setTexture(_atlas.get());
				postRenderer.setTint(colors[index]);
			}
			_viewport.frame(
				_contextPreview ? spk::Vector3{1.5f, 0.5f, 1.5f} : spk::Vector3{2.0f, 0.5f, 0.5f},
				_contextPreview ? 6.0f : 6.5f);
			_services.report("Preview rebuilt for " + _document.selectedId());
		}
		catch (const std::exception &exception)
		{
			_services.report(std::string("Preview error: ") + exception.what());
		}
	}

	void VoxelModelerPage::_select(const std::string &p_id)
	{
		_document.select(p_id);
		_rebuildProperties();
		_rebuildPreview();
	}

	void VoxelModelerPage::_changed(bool p_rebuildProperties)
	{
		_document.markSelectedChanged();
		_state.markChanged();
		if (p_rebuildProperties)
		{
			_rebuildProperties();
		}
		_rebuildPreview();
	}

	nlohmann::json VoxelModelerPage::_newVoxel()
	{
		return {
			{"version", 1},
			{"traversal", "solid"},
			{"tags", nlohmann::json::array()},
			{"shape", _shapeDefaults("cube")}};
	}

	nlohmann::json VoxelModelerPage::_shapeDefaults(const std::string &p_type)
	{
		if (p_type == "cube")
		{
			return {{"type", p_type}, {"textures", {{"top", {0, 0}}, {"bottom", {0, 0}}, {"side", {0, 0}}}}};
		}
		if (p_type == "slab")
		{
			return {{"type", p_type}, {"height", 0.5f}, {"textures", {{"top", {0, 0}}, {"bottom", {0, 0}}, {"side", {0, 0}}}}};
		}
		if (p_type == "slope")
		{
			return {{"type", p_type}, {"textures", {{"slope", {0, 0}}, {"back", {0, 0}}, {"bottom", {0, 0}}, {"sideLeft", {0, 0}}, {"sideRight", {0, 0}}}}};
		}
		if (p_type == "stair")
		{
			return {{"type", p_type}, {"stepCount", 2}, {"textures", {{"top", {0, 0}}, {"riser", {0, 0}}, {"back", {0, 0}}, {"bottom", {0, 0}}, {"sideLeft", {0, 0}}, {"sideRight", {0, 0}}}}};
		}
		if (p_type == "crossPlane")
		{
			return {{"type", p_type}, {"textures", {{"plane", {0, 0}}}}};
		}
		throw std::invalid_argument("Unknown voxel shape type '" + p_type + "'");
	}

	std::string VoxelModelerPage::title() const
	{
		return "Voxel Modeler";
	}

	bool VoxelModelerPage::hasUnsavedChanges() const
	{
		return _document.dirty();
	}

	void VoxelModelerPage::save()
	{
		_document.save(_services.writer());
		_services.load();
		_state.markSaved();
		_services.report("Saved voxel definitions and reloaded registries");
	}

	void VoxelModelerPage::reload()
	{
		_document.load(_services.dataDirectory() / "voxels");
		_state.markSaved();
		_rebuildList();
		_rebuildProperties();
		_rebuildPreview();
		_services.report("Voxel definitions reloaded");
	}
}
