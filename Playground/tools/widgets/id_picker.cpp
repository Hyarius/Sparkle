#include "tools/widgets/id_picker.hpp"

#include <algorithm>
#include <cctype>
#include <utility>

namespace
{
	[[nodiscard]] std::string lowercase(std::string p_value)
	{
		std::ranges::transform(p_value, p_value.begin(), [](unsigned char p_character) {
			return static_cast<char>(std::tolower(p_character));
		});
		return p_value;
	}
}

namespace pg::tools
{
	IdPicker::IdPicker(const std::string &p_name, spk::Widget *p_parent) :
		spk::Widget(p_name, p_parent),
		_search(p_name + "/Search", this),
		_selection(p_name + "/Selection", "(none)", this)
	{
		_search.setPlaceholder("Filter ids...");
		_search.subscribeToEdition([this](const spk::Font::Text &) {
			_filter(_search.textAsUTF8());
		}).relinquish();
		_selection.subscribeToClick([this]() {
			if (_matches.empty())
			{
				return;
			}
			auto iterator = std::ranges::find(_matches, _selected);
			const std::size_t index = iterator == _matches.end()
				? 0u
				: (static_cast<std::size_t>(std::distance(_matches.begin(), iterator)) + 1u) % _matches.size();
			select(_matches[index], true);
		}).relinquish();
		_search.activate();
		_selection.activate();
		_layout.setElementPadding({4, 4});
		_layout.addWidget(&_search, spk::Layout::SizePolicy::Minimum);
		_layout.addWidget(&_selection, spk::Layout::SizePolicy::Minimum);
		configureMinimalSizeGenerator([this]() { return _layout.minimalSize(); });
	}

	void IdPicker::_onGeometryChange()
	{
		_layout.setGeometry(geometry().atOrigin());
	}

	void IdPicker::_filter(const std::string &p_query)
	{
		const std::string query = lowercase(p_query);
		_matches.clear();
		for (const std::string &id : _ids)
		{
			if (query.empty() || lowercase(id).find(query) != std::string::npos)
			{
				_matches.push_back(id);
			}
		}
	}

	void IdPicker::setIds(std::vector<std::string> p_ids)
	{
		std::ranges::sort(p_ids);
		_ids = std::move(p_ids);
		_filter(_search.textAsUTF8());
		if (!_selected.empty() && !std::ranges::contains(_ids, _selected))
		{
			select({}, false);
		}
	}

	void IdPicker::select(const std::string &p_id, bool p_notify)
	{
		_selected = p_id;
		_selection.setText(_selected.empty() ? "(none)" : _selected);
		if (p_notify && _callback)
		{
			_callback(_selected);
		}
	}

	void IdPicker::subscribeToSelection(std::function<void(const std::string &)> p_callback)
	{
		_callback = std::move(p_callback);
	}

	const std::string &IdPicker::selected() const noexcept
	{
		return _selected;
	}

	const std::vector<std::string> &IdPicker::matches() const noexcept
	{
		return _matches;
	}
}
