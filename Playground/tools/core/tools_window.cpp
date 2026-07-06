#include "tools/core/tools_window.hpp"

#include <stdexcept>
#include <utility>

namespace pg::tools
{
	ToolsWindow::ToolsWindow(
		const std::string &p_name,
		spk::Widget *p_parent,
		ToolServices &p_services) :
		spk::Widget(p_name, p_parent),
		_services(p_services),
		_background(p_name + "/Background", this),
		_tabBar(p_name + "/Tabs", this),
		_pageArea(p_name + "/Pages", this),
		_status(p_name + "/Status", "Ready", this),
		_unsavedPrompt(p_name + "/UnsavedPrompt", this)
	{
		_background.activate();
		_tabBar.activate();
		_pageArea.activate();
		_status.activate();
		_tabLayout.setElementPadding({4, 4});
		_layout.setElementPadding({4, 4});
		_layout.addWidget(&_tabBar, spk::Layout::SizePolicy::Fixed)->setSize({0, 38});
		_layout.addWidget(&_pageArea);
		_layout.addWidget(&_status, spk::Layout::SizePolicy::Fixed)->setSize({0, 28});
		_services.setStatusSink([this](const std::string &p_message) { setStatus(p_message); });
	}

	void ToolsWindow::_onGeometryChange()
	{
		_background.setGeometry(geometry().atOrigin());
		_layout.setGeometry(geometry().atOrigin().shrink({4, 4}));
		_tabLayout.setGeometry(_tabBar.geometry().atOrigin());
		for (const auto &page : _pages)
		{
			page->setGeometry(_pageArea.geometry().atOrigin());
		}
		const unsigned int promptWidth = std::min(520u, geometry().width());
		const unsigned int promptHeight = std::min(220u, geometry().height());
		_unsavedPrompt.setGeometry(spk::Rect2D(
			{static_cast<int>((geometry().width() - promptWidth) / 2), static_cast<int>((geometry().height() - promptHeight) / 2)},
			{promptWidth, promptHeight}));
	}

	void ToolsWindow::addPage(std::unique_ptr<IEditorPage> p_page)
	{
		if (p_page == nullptr)
		{
			throw std::invalid_argument("Cannot add a null editor page");
		}
		const std::size_t index = _pages.size();
		auto button = std::make_unique<spk::PushButton>(name() + "/Tab/" + p_page->title(), p_page->title(), &_tabBar);
		button->subscribeToClick([this, index]() { selectPage(index); }).relinquish();
		button->activate();
		_tabLayout.addWidget(button.get(), spk::Layout::SizePolicy::Minimum);
		p_page->deactivate();
		_pages.push_back(std::move(p_page));
		_tabButtons.push_back(std::move(button));
		if (_activePage == static_cast<std::size_t>(-1))
		{
			_showPage(0);
		}
		_onGeometryChange();
	}

	void ToolsWindow::_showPage(std::size_t p_index)
	{
		if (_activePage != static_cast<std::size_t>(-1))
		{
			_pages[_activePage]->deactivate();
		}
		_activePage = p_index;
		_pages[_activePage]->activate();
		setStatus(_pages[_activePage]->title());
	}

	void ToolsWindow::selectPage(std::size_t p_index)
	{
		if (p_index >= _pages.size() || p_index == _activePage)
		{
			return;
		}
		if (_activePage != static_cast<std::size_t>(-1) && _pages[_activePage]->hasUnsavedChanges())
		{
			const std::size_t current = _activePage;
			_unsavedPrompt.setText("Save changes in " + _pages[current]->title() + " before switching tabs?");
			_unsavedPrompt.configure(
				"Save", [this, current, p_index]() {
					_pages[current]->save();
					_unsavedPrompt.deactivate();
					_showPage(p_index);
				},
				"Discard", [this, current, p_index]() {
					_pages[current]->reload();
					_unsavedPrompt.deactivate();
					_showPage(p_index);
				});
			_unsavedPrompt.activate();
			return;
		}
		_showPage(p_index);
	}

	void ToolsWindow::setStatus(const std::string &p_message)
	{
		_status.setText(p_message);
	}

	spk::Widget &ToolsWindow::pageArea() noexcept
	{
		return _pageArea;
	}
}
