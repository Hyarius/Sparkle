#pragma once

#include "tools/core/feat_board_document.hpp"
#include "tools/core/i_editor_page.hpp"
#include "tools/core/tool_services.hpp"
#include "tools/widgets/graph_canvas.hpp"
#include "tools/widgets/property_panel.hpp"

#include <filesystem>
#include <memory>
#include <string>
#include <vector>

#include <sparkle.hpp>

namespace pg::tools
{
	class FeatBoardEditorPage : public IEditorPage
	{
	private:
		ToolServices &_services;
		FeatBoardDocument _document;
		spk::Panel _background;
		spk::HorizontalLayout _mainLayout;
		spk::Widget _leftPane;
		spk::VerticalLayout _leftLayout;
		spk::PushButton _newButton;
		spk::PushButton _saveButton;
		spk::PushButton _frameButton;
		std::vector<std::unique_ptr<spk::PushButton>> _boardButtons;
		GraphCanvas _canvas;
		PropertyPanel _properties;

		void _onGeometryChange() override;
		void _rebuildBoardList();
		void _rebuildGraph(bool p_frame = false);
		void _rebuildProperties();
		void _selectBoard(const std::filesystem::path &p_file);
		void _newBoard();
		void _changed(bool p_graph = false, bool p_properties = false);
		void _addJsonFields(nlohmann::json &p_object, const std::string &p_prefix);
		[[nodiscard]] std::vector<std::string> _formIds() const;

	public:
		FeatBoardEditorPage(const std::string &p_name, spk::Widget *p_parent, ToolServices &p_services);

		[[nodiscard]] std::string title() const override;
		[[nodiscard]] bool hasUnsavedChanges() const override;
		void save() override;
		void reload() override;
	};
}
