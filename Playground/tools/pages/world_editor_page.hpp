#pragma once

#include "tools/core/edit_camera.hpp"
#include "tools/core/i_editor_page.hpp"
#include "tools/core/tool_services.hpp"
#include "tools/widgets/id_picker.hpp"
#include "tools/widgets/property_panel.hpp"
#include "tools/widgets/viewport3d.hpp"
#include "voxel/voxel_grid.hpp"
#include "world/world_raycaster.hpp"

#include <filesystem>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include <nlohmann/json.hpp>
#include <sparkle.hpp>

namespace pg
{
	class VoxelWorld;
	struct PrefabDefinition;
}

namespace pg::tools
{
	class WorldDocument
	{
	public:
		enum class Kind
		{
			Map,
			Prefab
		};

	private:
		Kind _kind = Kind::Map;
		std::filesystem::path _file;
		std::filesystem::path _sourceFile;
		std::string _id;
		nlohmann::json _json;
		VoxelGrid _grid;
		const VoxelRegistry *_voxels = nullptr;
		bool _dirty = false;

		[[nodiscard]] std::string _paletteToken(const VoxelCell &p_cell);
		void _setCellOverride(const spk::Vector3Int &p_at, const VoxelCell &p_cell);
		void _writeDenseCells();

	public:
		void load(const std::filesystem::path &p_file, Kind p_kind, const Registries &p_registries);
		void create(
			const std::filesystem::path &p_directory,
			std::string p_id,
			Kind p_kind,
			const VoxelRegistry &p_voxels,
			const spk::Vector3Int &p_size);

		[[nodiscard]] Kind kind() const noexcept;
		[[nodiscard]] const std::string &id() const noexcept;
		[[nodiscard]] const std::filesystem::path &file() const noexcept;
		[[nodiscard]] const nlohmann::json &json() const noexcept;
		[[nodiscard]] nlohmann::json &editJson() noexcept;
		[[nodiscard]] const VoxelGrid &grid() const noexcept;
		[[nodiscard]] bool dirty() const noexcept;
		void markChanged() noexcept;
		void rename(const std::string &p_id);
		void resize(const spk::Vector3Int &p_size);
		bool setCell(const spk::Vector3Int &p_at, const VoxelCell &p_cell);
		void fillBox(const spk::Vector3Int &p_first, const spk::Vector3Int &p_second, const VoxelCell &p_cell);
		void addStamp(
			const std::string &p_prefabId,
			const spk::Vector3Int &p_at,
			VoxelOrientation p_orientation,
			const PrefabDefinition &p_prefab);
		void save(const JsonWriter &p_writer);

		[[nodiscard]] static std::vector<spk::Vector3Int> boxCells(
			const spk::Vector3Int &p_first,
			const spk::Vector3Int &p_second);
		[[nodiscard]] static std::optional<spk::Vector3Int> adjacentCell(const VoxelHit &p_hit);
	};

	class WorldEditorPage : public IEditorPage
	{
	private:
		enum class ToolMode
		{
			Place,
			Erase,
			BoxFill,
			Marker,
			Portal,
			Trainer,
			Stamp,
			Anchor
		};

		enum class SelectionKind
		{
			None,
			Marker,
			Portal,
			Trainer,
			Stamp,
			Anchor
		};

		ToolServices &_services;
		WorldDocument _document;
		EditCamera _editCamera;
		spk::Panel _background;
		spk::HorizontalLayout _mainLayout;
		spk::Widget _leftPane;
		spk::VerticalLayout _leftLayout;
		spk::PushButton _newMapButton;
		spk::PushButton _newPrefabButton;
		spk::PushButton _saveButton;
		spk::PushButton _toolButton;
		IdPicker _voxelPicker;
		IdPicker _prefabPicker;
		std::vector<std::unique_ptr<spk::PushButton>> _documentButtons;
		Viewport3D _viewport;
		PropertyPanel _properties;
		std::shared_ptr<spk::SpriteSheet> _atlas;
		std::unique_ptr<VoxelWorld> _world;
		ToolMode _tool = ToolMode::Place;
		SelectionKind _selection = SelectionKind::None;
		std::size_t _selectionIndex = 0;
		std::optional<spk::Vector3Int> _boxStart;

		void _onGeometryChange() override;
		void _onMouseMovedEvent(spk::MouseMovedEvent &p_event) override;
		void _onMouseButtonPressedEvent(spk::MouseButtonPressedEvent &p_event) override;
		void _onKeyPressedEvent(spk::KeyPressedEvent &p_event) override;
		void _rebuildDocumentList();
		void _rebuildWorld();
		void _rebuildProperties();
		void _selectDocument(const std::filesystem::path &p_file, WorldDocument::Kind p_kind);
		void _newDocument(WorldDocument::Kind p_kind);
		void _cycleTool();
		void _applyTool(const std::optional<VoxelHit> &p_hit, const WorldRay &p_ray);
		void _addPlacedElement(const spk::Vector3Int &p_at);
		void _changed(bool p_rebuildWorld = true);
		[[nodiscard]] VoxelCell _brushCell() const;
		[[nodiscard]] std::optional<spk::Vector3Int> _groundCell(const WorldRay &p_ray) const;

	public:
		WorldEditorPage(const std::string &p_name, spk::Widget *p_parent, ToolServices &p_services);
		~WorldEditorPage() override;

		[[nodiscard]] std::string title() const override;
		[[nodiscard]] bool hasUnsavedChanges() const override;
		void save() override;
		void reload() override;
	};
}
