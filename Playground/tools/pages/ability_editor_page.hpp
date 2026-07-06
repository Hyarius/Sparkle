#pragma once

#include "tools/core/content_document.hpp"
#include "tools/core/i_editor_page.hpp"
#include "tools/core/tool_services.hpp"
#include "tools/widgets/property_panel.hpp"

#include <filesystem>
#include <memory>
#include <string>
#include <vector>

#include <sparkle.hpp>

namespace pg::tools
{
	class AbilityEditorPage : public IEditorPage
	{
	private:
		ToolServices &_services;
		ContentDocument _document;
		spk::Panel _background;
		spk::HorizontalLayout _mainLayout;
		spk::Widget _leftPane;
		spk::VerticalLayout _leftLayout;
		spk::PushButton _domainButton;
		spk::PushButton _newButton;
		spk::PushButton _saveButton;
		std::vector<std::unique_ptr<spk::PushButton>> _documentButtons;
		PropertyPanel _properties;
		spk::TextArea _rulesPreview;
		std::shared_ptr<spk::SpriteSheet> _icons;
		ContentDocument::Domain _domain = ContentDocument::Domain::Ability;

		void _onGeometryChange() override;
		void _rebuildList();
		void _rebuildProperties();
		void _refreshPreview();
		void _selectDocument(const std::filesystem::path &p_file, ContentDocument::Domain p_domain);
		void _newDocument();
		void _toggleDomain();
		void _changed(bool p_rebuildProperties = false);
		void _addJsonFields(nlohmann::json &p_object, const std::string &p_prefix);

	public:
		AbilityEditorPage(const std::string &p_name, spk::Widget *p_parent, ToolServices &p_services);

		[[nodiscard]] std::string title() const override;
		[[nodiscard]] bool hasUnsavedChanges() const override;
		void save() override;
		void reload() override;
	};
}
