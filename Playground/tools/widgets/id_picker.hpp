#pragma once

#include <functional>
#include <string>
#include <vector>

#include <sparkle.hpp>

namespace pg::tools
{
	class IdPicker : public spk::Widget
	{
	private:
		spk::VerticalLayout _layout;
		spk::TextEdit _search;
		spk::PushButton _selection;
		std::vector<std::string> _ids;
		std::vector<std::string> _matches;
		std::string _selected;
		std::function<void(const std::string &)> _callback;

		void _onGeometryChange() override;
		void _filter(const std::string &p_query);

	public:
		IdPicker(const std::string &p_name, spk::Widget *p_parent = nullptr);

		void setIds(std::vector<std::string> p_ids);
		void select(const std::string &p_id, bool p_notify = false);
		void subscribeToSelection(std::function<void(const std::string &)> p_callback);
		[[nodiscard]] const std::string &selected() const noexcept;
		[[nodiscard]] const std::vector<std::string> &matches() const noexcept;
	};
}
