#include <iostream>

#include <sparkle.hpp>

class Page : public spk::Widget
{

public:
	Page(const std::string &p_name, spk::Widget *p_parent) :
		spk::Widget(p_name, p_parent)
	{

	}

	virtual std::string pageName() const = 0;
};

class EventCenter
{
public:
	using PageEventProvider = spk::ContractProvider<const std::shared_ptr<Page> &>;
	using PageEventCallback = PageEventProvider::Callback;
	using PageEventContract = PageEventProvider::Contract;

private:
	static inline PageEventProvider _pageSelectionRequestProvider;

public:
	static PageEventContract subscribeToPageSelection(PageEventCallback p_callback)
	{
		return _pageSelectionRequestProvider.subscribe(std::move(p_callback));
	}

	static void notifyPageSelection(const std::shared_ptr<Page> &p_page)
	{
		_pageSelectionRequestProvider.trigger(p_page);
	}
};

class Header : public spk::Widget
{
private:
	EventCenter::PageEventContract _onPageSelectionContract;

	spk::Panel _background;

	spk::HorizontalLayout _hLayout;

	spk::TextLabel _pageLabel;

	void _onGeometryChange()
	{
		_background.setGeometry(geometry().atOrigin());
		_hLayout.setGeometry(geometry().atOrigin());
	}

public:
	Header(const std::string &p_name, spk::Widget *p_parent) :
		spk::Widget(p_name, p_parent),
		_background(p_name + "/Background", this),
		_pageLabel(p_name + "/PageLabel", this)
	{
		_background.setMinimalSize({0, 40});

		_onPageSelectionContract = EventCenter::subscribeToPageSelection([&](const std::shared_ptr<Page>& p_page){_pageLabel.setText(p_page->pageName());});

		_background.activate();
		_pageLabel.activate();

		_hLayout.addWidget(&_pageLabel);

		configureMinimalSizeGenerator([&]() -> spk::Vector2UInt {
			return spk::Vector2UInt::max(_hLayout.minimalSize(), _background.minimalSize());
		});
	}
};

class NavigationWidget : public spk::Widget
{
private:
	spk::Panel _background;

	spk::VerticalLayout _vLayout;

	std::unordered_map<std::shared_ptr<Page>, std::shared_ptr<spk::PushButton>> _navButtons;
	spk::SpacerWidget _bottomSpace;

	void _onGeometryChange()
	{
		_background.setGeometry(geometry().atOrigin());
		_composeLayout();
		_vLayout.setGeometry(geometry().atOrigin().shrink(static_cast<const spk::Vector2Int>(_vLayout.elementPadding())));
	}

	void _composeLayout()
	{
		_vLayout.clear();

		for (auto &[page, button] : _navButtons)
		{
			_vLayout.addWidget(button.get(), spk::Layout::SizePolicy::Minimum);
		}

		_vLayout.addWidget(&_bottomSpace);

		releaseMinimalSize();
	}

public:
	NavigationWidget(const std::string &p_name, spk::Widget *p_parent) :
		spk::Widget(p_name, p_parent),
		_background(p_name + "/Background", this),
		_bottomSpace(p_name + "/Spacer", this)
	{
		_background.activate();

		_vLayout.setElementPadding({5, 5});

		configureMinimalSizeGenerator([&]() -> spk::Vector2UInt {
			return spk::Vector2UInt::max(
				_vLayout.minimalSize() + (_vLayout.elementPadding() * 2u),
				_background.minimalSize()
			);
		});
	}

	void addNavigationButton(const std::shared_ptr<Page> &p_page)
	{
		_navButtons[p_page] = std::make_shared<spk::PushButton>(name() + "/" + p_page->name() + "Button", this);
		_navButtons[p_page]->setText(p_page->pageName());
		_navButtons[p_page]->subscribeToClick([p_page]() {
			EventCenter::notifyPageSelection(p_page);
		}).relinquish();
		_navButtons[p_page]->activate();

		_onGeometryChange();
	}
};

class PageRegistry : public spk::Widget
{
private:
	EventCenter::PageEventContract _onPageSelectionContract;
	spk::Panel _background;
	std::shared_ptr<Page> _activatedPage;
	std::unordered_map<std::string, std::shared_ptr<Page>> _pages;

	void _onGeometryChange()
	{
		_background.setGeometry(geometry().atOrigin());
		for (auto &[name, page] : _pages)
		{
			page->setGeometry(geometry().atOrigin().shrink(static_cast<spk::Vector2Int>(_background.cornerSize())));
		}
	}

	void _activatePage(const std::shared_ptr<Page> &p_page)
	{
		if (_activatedPage != nullptr)
		{
			_activatedPage->deactivate();
		}

		_activatedPage = p_page;

		if (_activatedPage != nullptr)
		{
			_activatedPage->activate();
		}
	}

public:
	PageRegistry(const std::string &p_name, spk::Widget *p_parent) :
		spk::Widget(p_name, p_parent),
		_background(p_name + "/Background", this)
	{
		_background.activate();
		_onPageSelectionContract = EventCenter::subscribeToPageSelection([&](const std::shared_ptr<Page>& p_page){_activatePage(p_page);});

		configureMinimalSizeGenerator([&]() -> spk::Vector2UInt {
			spk::Vector2UInt result = {0, 0};
			for (const auto& [name, page] : _pages)
			{
				result = spk::Vector2UInt::max(page->minimalSize(), result);
			}
			return (result);
		});
	}

	void registerPage(const std::shared_ptr<Page> &p_page)
	{
		if (_pages.contains(p_page->name()) == true)
		{
			throw std::runtime_error("Page [" + p_page->name() + "] already registered");
		}

		_pages.emplace(p_page->name(), p_page);

		releaseMinimalSize();

		if (_activatedPage == nullptr)
		{
			EventCenter::notifyPageSelection(p_page);
		}
	}
};

class Body : public spk::Widget
{
private:
	spk::HorizontalLayout _hLayout;

	NavigationWidget _navWidget;
	PageRegistry _pageRegistry;

	void _onGeometryChange()
	{
		_hLayout.setGeometry(geometry().atOrigin());
	}

public:
	Body(const std::string &p_name, spk::Widget *p_parent) :
		spk::Widget(p_name, p_parent),
		_navWidget(p_name + "/NavWidget", this),
		_pageRegistry(p_name + "/PageRegistry", this)
	{
		_navWidget.activate();
		_pageRegistry.activate();

		_hLayout.setElementPadding({5, 5});
		_hLayout.addWidget(&_navWidget, spk::Layout::SizePolicy::Minimum);
		_hLayout.addWidget(&_pageRegistry);

		configureMinimalSizeGenerator([&]() -> spk::Vector2UInt {
			return _hLayout.minimalSize();
		});
	}

	template <typename TPageType>
	void addPage(const std::string& p_name)
	{
		std::shared_ptr<TPageType> newPage = std::make_shared<TPageType>(p_name, &_pageRegistry);

		_pageRegistry.registerPage(newPage);
		_navWidget.addNavigationButton(newPage);
	}
};

class TextBasedWidgetPage : public Page
{
private:
	spk::GridLayout _gLayout;

	spk::TextLabel _label;
	spk::TextEdit _edit;
	spk::TextArea _textArea;
	spk::DynamicTextLabel _dynamicTextArea;

	spk::SpacerWidget _spacer;

	void _onGeometryChange() override
	{
		_gLayout.setGeometry(geometry().atOrigin());
	}

public:
	TextBasedWidgetPage(const std::string &p_name, spk::Widget *p_parent) :
		Page(p_name, p_parent),
		_label(p_name + "/Label", this),
		_edit(p_name + "/Edit", this),
		_textArea(p_name + "/TextArea", this),
		_dynamicTextArea(p_name + "/DynamicLabel", this),
		_spacer(p_name + "/Spacer", this)
	{
		_label.setText("Exemple of text label");
		_label.activate();

		_edit.setPlaceholder("Placeholder text");
		_edit.activate();

		_textArea.setText("TextArea on multiple line\nIt's suppose to manage return automaticaly without need of '\\n' but it will also support it.\nFor exemple, this\nwill be separated.");
		_textArea.activate();

		_dynamicTextArea.setTextProducer([&]() -> std::string {
			return "Program duration : " + spk::TimeUtils::getTime().toString();
		});
		_dynamicTextArea.activate();

		_gLayout.setElementPadding({5, 5});
		_gLayout.setWidget(0, 0, &_label, spk::Layout::SizePolicy::Minimum);
		_gLayout.setWidget(0, 1, &_edit, spk::Layout::SizePolicy::Minimum);
		_gLayout.setWidget(0, 2, &_textArea, spk::Layout::SizePolicy::Minimum);
		_gLayout.setWidget(0, 3, &_dynamicTextArea, spk::Layout::SizePolicy::Minimum);
		_gLayout.setWidget(10, 10, &_spacer, spk::Layout::SizePolicy::Extend);

		configureMinimalSizeGenerator([&]() -> spk::Vector2UInt {
			return _gLayout.minimalSize();
		});
	}

	std::string pageName() const override
	{
		return ("Text Page");
	}
};

class InteractionPage : public Page
{
private:
	spk::GridLayout _gLayout;

	spk::PushButton _pushButton;
	int _nbClick = 0;
	spk::TextLabel _pushButtonLabelOutput;

	spk::SpacerWidget _spacer;

	void _onGeometryChange() override
	{
		Page::_onGeometryChange();
		_gLayout.setGeometry(geometry().atOrigin());
	}

	void _onButtonClick()
	{
		_nbClick++;
		_pushButtonLabelOutput.setText("Nb Click : " + std::to_string(_nbClick));
		_onGeometryChange();
	}

public:
	InteractionPage(const std::string &p_name, spk::Widget *p_parent) :
		Page(p_name, p_parent),
		_pushButton(p_name + "/PushButton", this),
		_pushButtonLabelOutput(p_name + "/PushButtonLabelOutput", this),
		_spacer(p_name + "/Spacer", this)
	{
		_pushButton.setText("Click me");
		_pushButton.subscribeToClick([&](){_onButtonClick();});
		_pushButton.activate();

		_onButtonClick();

		_gLayout.setElementPadding({5, 5});
		_gLayout.setWidget(0, 0, &_pushButton, spk::Layout::SizePolicy::Minimum);
		_gLayout.setWidget(0, 1, &_pushButtonLabelOutput, spk::Layout::SizePolicy::Minimum);
		_gLayout.setWidget(10, 10, &_spacer, spk::Layout::SizePolicy::Extend);

		configureMinimalSizeGenerator([&]() -> spk::Vector2UInt {
			return _gLayout.minimalSize();
		});
	}

	std::string pageName() const override
	{
		return ("Interaction Page");
	}
};

class MainWidget : public spk::Widget
{
private:
	spk::VerticalLayout _vLayout;

	Header _header;
	Body _body;

	void _onGeometryChange()
	{
		_vLayout.setGeometry(geometry().atOrigin().shrink(static_cast<const spk::Vector2Int>(_vLayout.elementPadding())));
	}

public:
	MainWidget(const std::string &p_name, spk::Widget *p_parent) :
		spk::Widget(p_name, p_parent),
		_header(p_name + "/Header", this),
		_body(p_name + "/Body", this)
	{
		_header.activate();
		_body.activate();

		_body.addPage<TextBasedWidgetPage>("TextBasedWidgetPage");
		_body.addPage<InteractionPage>("InteractionPage");

		_vLayout.setElementPadding({5, 5});
		_vLayout.addWidget(&_header, spk::Layout::SizePolicy::Minimum);
		_vLayout.addWidget(&_body, spk::Layout::SizePolicy::Extend);

		configureMinimalSizeGenerator([&]() -> spk::Vector2UInt {
			return _vLayout.minimalSize();
		});
	}
};

int main()
{
	try
	{
		spk::Application application;

		spk::WindowHandle window = application.createWindow(
			"UIShowcaseID",
			spk::Window::Configuration{
				.rect = spk::Rect2D(80, 80, 1280, 800),
				.title = "Sparkle UI Showcase"});

		MainWidget mainWidget = MainWidget("./MainWidget", &(window.centralWidget()));
		mainWidget.setGeometry(window.centralWidget().geometry());
		mainWidget.activate();


		std::cout << "Sparkle UI Showcase opened. Close the window to exit." << std::endl;
		return application.run();
	} catch (const std::exception &exception)
	{
		std::cerr << "Sparkle UI Showcase failed: " << exception.what() << std::endl;
		return 1;
	}
}
