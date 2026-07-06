#pragma once

#include "tools/core/content_document.hpp"
#include "tools/core/i_editor_page.hpp"
#include "tools/core/tool_services.hpp"
#include "tools/widgets/property_panel.hpp"
#include "tools/widgets/viewport3d.hpp"

#include <filesystem>
#include <memory>
#include <string>
#include <vector>

#include <sparkle.hpp>

namespace pg::tools
{
	class EncounterEditorPage : public IEditorPage
	{
	private:
		ToolServices &_services;
		ContentDocument _document;
		spk::Panel _background;
		spk::HorizontalLayout _mainLayout;
		spk::Widget _leftPane;
		spk::VerticalLayout _leftLayout;
		spk::PushButton _newButton;
		spk::PushButton _saveButton;
		spk::PushButton _tierButton;
		spk::PushButton _addTeamButton;
		spk::PushButton _deleteTeamButton;
		std::vector<std::unique_ptr<spk::PushButton>> _listButtons;
		Viewport3D _preview;
		PropertyPanel _properties;
		std::shared_ptr<spk::SpriteSheet> _atlas;
		std::size_t _tier = 0;
		std::size_t _team = 0;

		void _onGeometryChange() override;
		void _rebuildList();
		void _rebuildProperties();
		void _rebuildPreview();
		void _selectTable(const std::filesystem::path &p_file);
		void _newTable();
		void _cycleTier();
		void _addTeam();
		void _deleteTeam();
		void _changed(bool p_properties = false);
		[[nodiscard]] std::string _tierName() const;
		[[nodiscard]] nlohmann::json *_selectedTeam();

	public:
		EncounterEditorPage(const std::string &p_name, spk::Widget *p_parent, ToolServices &p_services);

		[[nodiscard]] std::string title() const override;
		[[nodiscard]] bool hasUnsavedChanges() const override;
		void save() override;
		void reload() override;
	};
}
