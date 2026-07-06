#pragma once

#include "tools/core/i_editor_page.hpp"
#include "tools/core/tool_services.hpp"
#include "tools/widgets/property_panel.hpp"
#include "tools/widgets/viewport3d.hpp"

#include <filesystem>
#include <map>
#include <memory>
#include <set>
#include <string>
#include <vector>

#include <nlohmann/json.hpp>
#include <sparkle.hpp>

namespace pg::tools
{
	class VoxelModelDocument
	{
	private:
		std::filesystem::path _directory;
		std::map<std::string, nlohmann::json> _definitions;
		std::set<std::string> _dirty;
		std::set<std::string> _deleted;
		std::string _selected;

	public:
		void load(const std::filesystem::path &p_directory);
		[[nodiscard]] std::vector<std::string> ids() const;
		[[nodiscard]] bool contains(const std::string &p_id) const;
		void select(const std::string &p_id);
		[[nodiscard]] const std::string &selectedId() const noexcept;
		[[nodiscard]] nlohmann::json &selected();
		[[nodiscard]] const nlohmann::json &selected() const;
		std::string create(std::string p_baseId, nlohmann::json p_definition);
		void renameSelected(const std::string &p_id);
		std::string duplicateSelected();
		void deleteSelected();
		void markSelectedChanged();
		void save(const JsonWriter &p_writer);
		[[nodiscard]] bool dirty() const noexcept;
	};

	class VoxelModelerPage : public IEditorPage
	{
	private:
		ToolServices &_services;
		VoxelModelDocument _document;
		spk::Panel _background;
		spk::HorizontalLayout _mainLayout;
		spk::Widget _listPane;
		spk::VerticalLayout _listLayout;
		spk::PushButton _newButton;
		spk::PushButton _duplicateButton;
		spk::PushButton _deleteButton;
		spk::PushButton _saveButton;
		spk::PushButton _previewModeButton;
		std::vector<std::unique_ptr<spk::PushButton>> _definitionButtons;
		Viewport3D _viewport;
		PropertyPanel _properties;
		std::shared_ptr<spk::SpriteSheet> _atlas;
		bool _contextPreview = false;
		std::filesystem::path _previewDirectory;

		void _onGeometryChange() override;
		void _rebuildList();
		void _rebuildProperties();
		void _rebuildPreview();
		void _select(const std::string &p_id);
		void _changed(bool p_rebuildProperties = false);
		[[nodiscard]] static nlohmann::json _newVoxel();
		[[nodiscard]] static nlohmann::json _shapeDefaults(const std::string &p_type);

	public:
		VoxelModelerPage(const std::string &p_name, spk::Widget *p_parent, ToolServices &p_services);
		~VoxelModelerPage() override;

		[[nodiscard]] std::string title() const override;
		[[nodiscard]] bool hasUnsavedChanges() const override;
		void save() override;
		void reload() override;
	};
}
