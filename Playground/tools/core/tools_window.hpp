#pragma once

#include "tools/core/i_editor_page.hpp"
#include "tools/core/tool_services.hpp"

#include <cstddef>
#include <memory>
#include <string>
#include <vector>

#include <sparkle.hpp>

namespace pg::tools
{
	class ToolsWindow : public spk::Widget
	{
	private:
		ToolServices &_services;
		spk::Panel _background;
		spk::VerticalLayout _layout;
		spk::Widget _tabBar;
		spk::HorizontalLayout _tabLayout;
		spk::Widget _pageArea;
		spk::TextLabel _status;
		spk::RequestMessageBox _unsavedPrompt;
		std::vector<std::unique_ptr<IEditorPage>> _pages;
		std::vector<std::unique_ptr<spk::PushButton>> _tabButtons;
		std::size_t _activePage = static_cast<std::size_t>(-1);

		void _onGeometryChange() override;
		void _showPage(std::size_t p_index);

	public:
		ToolsWindow(const std::string &p_name, spk::Widget *p_parent, ToolServices &p_services);

		void addPage(std::unique_ptr<IEditorPage> p_page);
		void selectPage(std::size_t p_index);
		void setStatus(const std::string &p_message);
		[[nodiscard]] spk::Widget &pageArea() noexcept;
	};
}
