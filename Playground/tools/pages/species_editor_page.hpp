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
	class SpeciesEditorPage : public IEditorPage
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
		std::vector<std::unique_ptr<spk::PushButton>> _speciesButtons;
		Viewport3D _preview;
		PropertyPanel _properties;
		std::shared_ptr<spk::SpriteSheet> _atlas;

		void _onGeometryChange() override;
		void _rebuildList();
		void _rebuildProperties();
		void _rebuildPreview();
		void _selectSpecies(const std::filesystem::path &p_file);
		void _newSpecies();
		void _changed(bool p_properties = false, bool p_preview = false);
		void _addConditionFields(nlohmann::json &p_condition, const std::string &p_prefix);

	public:
		SpeciesEditorPage(const std::string &p_name, spk::Widget *p_parent, ToolServices &p_services);

		[[nodiscard]] std::string title() const override;
		[[nodiscard]] bool hasUnsavedChanges() const override;
		void save() override;
		void reload() override;
	};
}
