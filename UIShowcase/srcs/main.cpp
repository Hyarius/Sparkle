#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include <string_view>
#include <type_traits>
#include <unordered_map>
#include <utility>
#include <vector>

#include <sparkle.hpp>

namespace
{
	constexpr spk::Vector2UInt PagePadding = {8, 8};
	constexpr spk::Vector2UInt LargeControlSize = {420, 120};

	[[nodiscard]] std::string boolText(bool p_value)
	{
		return p_value ? "true" : "false";
	}
}

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

	void _onGeometryChange() override
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

		_onPageSelectionContract = EventCenter::subscribeToPageSelection(
			[&](const std::shared_ptr<Page> &p_page) {
				_pageLabel.setText(p_page->pageName());
			});

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

	std::vector<std::pair<std::shared_ptr<Page>, std::shared_ptr<spk::PushButton>>> _navButtons;
	spk::SpacerWidget _bottomSpace;

	void _onGeometryChange() override
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
			(void)page;
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
				_background.minimalSize());
		});
	}

	void addNavigationButton(const std::shared_ptr<Page> &p_page)
	{
		auto button = std::make_shared<spk::PushButton>(name() + "/" + p_page->name() + "Button", this);
		button->setText(p_page->pageName());
		button->subscribeToClick([p_page]() {
				  EventCenter::notifyPageSelection(p_page);
			  })
			.relinquish();
		button->activate();

		_navButtons.emplace_back(p_page, std::move(button));
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

	void _onGeometryChange() override
	{
		_background.setGeometry(geometry().atOrigin());
		for (auto &[name, page] : _pages)
		{
			(void)name;
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
		_onPageSelectionContract = EventCenter::subscribeToPageSelection(
			[&](const std::shared_ptr<Page> &p_page) {
				_activatePage(p_page);
			});

		configureMinimalSizeGenerator([&]() -> spk::Vector2UInt {
			spk::Vector2UInt result = {0, 0};
			for (const auto &[name, page] : _pages)
			{
				(void)name;
				result = spk::Vector2UInt::max(page->minimalSize(), result);
			}
			return result;
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

	void _onGeometryChange() override
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
	void addPage(const std::string &p_name)
	{
		std::shared_ptr<TPageType> newPage = std::make_shared<TPageType>(p_name, &_pageRegistry);

		_pageRegistry.registerPage(newPage);
		_navWidget.addNavigationButton(newPage);
	}
};

template <typename TContentType>
class Cell : public spk::Widget
{
	static_assert(std::is_base_of_v<spk::Widget, TContentType>);

private:
	static constexpr spk::Vector2UInt ContentPadding = {4, 4};

	spk::Panel _background;
	TContentType _content;

	[[nodiscard]] static spk::Vector2UInt _paddedSize(const spk::Vector2UInt &p_contentSize)
	{
		return p_contentSize + (ContentPadding * 2u);
	}

	[[nodiscard]] static spk::Vector2UInt _contentAvailableSize(const spk::Vector2UInt &p_availableSize)
	{
		const spk::Vector2UInt paddingSize = ContentPadding * 2u;
		return {
			(p_availableSize.x > paddingSize.x) ? p_availableSize.x - paddingSize.x : 0u,
			(p_availableSize.y > paddingSize.y) ? p_availableSize.y - paddingSize.y : 0u};
	}

	void _onGeometryChange() override
	{
		_background.setGeometry(geometry().atOrigin());
		_content.setGeometry(geometry().atOrigin().shrink(static_cast<const spk::Vector2Int>(ContentPadding)));
	}

public:
	template <typename... TArguments>
	Cell(const std::string &p_name, spk::Widget *p_parent, TArguments &&...p_arguments) :
		spk::Widget(p_name, p_parent),
		_background(
			p_name + "/Background",
			spk::WidgetStyle::Collection::style(spk::WidgetStyle::Collection::DefaultLight),
			this),
		_content(p_name + "/Content", std::forward<TArguments>(p_arguments)..., this)
	{
		_background.activate();
		_content.activate();

		configureMinimalSizeGenerator([this]() -> spk::Vector2UInt {
			return _paddedSize(_content.minimalSize());
		});

		activate();
	}

	[[nodiscard]] spk::Vector2UInt preferredSizeFor(const spk::Vector2UInt &p_availableSize) const override
	{
		return _paddedSize(_content.preferredSizeFor(_contentAvailableSize(p_availableSize)));
	}

	[[nodiscard]] TContentType &content()
	{
		return _content;
	}

	[[nodiscard]] const TContentType &content() const
	{
		return _content;
	}
};

class ShowcaseGridPage : public Page
{
private:
	bool _isFinished = false;

protected:
	spk::GridLayout _grid;
	spk::SpacerWidget _bottomFiller;
	std::vector<std::unique_ptr<spk::SpacerWidget>> _rowSpacers;
	std::vector<std::unique_ptr<spk::Widget>> _ownedWidgets;
	std::unordered_map<spk::Widget *, spk::Widget *> _cellByContent;
	size_t _nextRow = 0;

	void _onGeometryChange() override
	{
		_grid.setGeometry(geometry().atOrigin());
	}

	template <typename TWidget, typename... TArguments>
	TWidget &makeWidget(const std::string &p_suffix, TArguments &&...p_arguments)
	{
		auto cell = std::make_unique<Cell<TWidget>>(
			name() + "/" + p_suffix + "Cell",
			this,
			std::forward<TArguments>(p_arguments)...);

		Cell<TWidget> *cellPtr = cell.get();
		TWidget &result = cellPtr->content();
		_cellByContent.emplace(&result, cellPtr);
		_ownedWidgets.push_back(std::move(cell));
		return result;
	}

	spk::TextLabel &makeCaption(const std::string &p_caption)
	{
		spk::TextLabel &caption = makeWidget<spk::TextLabel>("Caption" + std::to_string(_nextRow));
		caption.setText(p_caption);
		caption.setPadding({0, 0});
		caption.setMinimalSize({170, 24});
		return caption;
	}

	spk::TextArea &makeNote(const std::string &p_suffix, std::string_view p_text)
	{
		spk::TextArea &note = makeWidget<spk::TextArea>(p_suffix);
		note.setText(p_text);
		note.setMinimalWidth(320);
		note.setMinimalSize({360, 70});
		return note;
	}

	[[nodiscard]] spk::Widget *_cellFor(spk::Widget &p_content)
	{
		auto iterator = _cellByContent.find(&p_content);
		if (iterator == _cellByContent.end())
		{
			return &p_content;
		}
		return iterator->second;
	}

	void addRow(
		const std::string &p_caption,
		spk::Widget &p_widget,
		spk::Layout::SizePolicy p_widgetPolicy = spk::Layout::SizePolicy::Minimum)
	{
		spk::TextLabel &caption = makeCaption(p_caption);
		spk::Widget *captionCell = _cellFor(caption);
		spk::Widget *widgetCell = _cellFor(p_widget);
		auto rowSpacer = std::make_unique<spk::SpacerWidget>(name() + "/RowSpacer" + std::to_string(_nextRow), this);
		spk::SpacerWidget *rowSpacerPtr = rowSpacer.get();
		_rowSpacers.push_back(std::move(rowSpacer));

		_grid.setWidget(0, _nextRow, captionCell, spk::Layout::SizePolicy::Minimum);
		_grid.setWidget(1, _nextRow, widgetCell, p_widgetPolicy);
		_grid.setWidget(2, _nextRow, rowSpacerPtr, spk::Layout::SizePolicy::HorizontalExtend);
		++_nextRow;
		releaseMinimalSize();
	}

	void addNoteRow(const std::string &p_caption, std::string_view p_text)
	{
		spk::TextArea &note = makeNote("Note" + std::to_string(_nextRow), p_text);
		addRow(p_caption, note, spk::Layout::SizePolicy::Minimum);
	}

	void finishPage()
	{
		if (_isFinished == true)
		{
			return;
		}

		_grid.setWidget(2, _nextRow, &_bottomFiller, spk::Layout::SizePolicy::Extend);
		_isFinished = true;
		releaseMinimalSize();
	}

public:
	ShowcaseGridPage(const std::string &p_name, spk::Widget *p_parent) :
		Page(p_name, p_parent),
		_bottomFiller(p_name + "/BottomFiller", this)
	{
		_grid.setElementPadding(PagePadding);

		configureMinimalSizeGenerator([&]() -> spk::Vector2UInt {
			return _grid.minimalSize();
		});
	}
};

class ScrollableDemoContent : public spk::Widget
{
private:
	spk::GridLayout _grid;
	std::vector<std::unique_ptr<spk::PushButton>> _cells;

	void _onGeometryChange() override
	{
		_grid.setGeometry(geometry().atOrigin());
	}

public:
	ScrollableDemoContent(const std::string &p_name, spk::Widget *p_parent) :
		spk::Widget(p_name, p_parent)
	{
		_grid.setElementPadding({6, 6});

		for (size_t row = 0; row < 6; ++row)
		{
			for (size_t column = 0; column < 4; ++column)
			{
				auto button = std::make_unique<spk::PushButton>(
					name() + "/Cell" + std::to_string(row) + "_" + std::to_string(column),
					"Cell " + std::to_string(column) + "," + std::to_string(row),
					this);
				button->setMinimalSize({130, 42});
				button->activate();

				_grid.setWidget(column, row, button.get(), spk::Layout::SizePolicy::Minimum);
				_cells.push_back(std::move(button));
			}
		}

		setMinimalSize({560, 300});
		activate();
	}
};

class LinearLayoutDemo : public spk::Widget
{
private:
	spk::Panel _background;
	spk::HorizontalLayout _layout;
	spk::PushButton _fixedButton;
	spk::PushButton _minimumButton;
	spk::PushButton _extendButton;
	spk::PushButton _horizontalExtendButton;

	void _onGeometryChange() override
	{
		_background.setGeometry(geometry().atOrigin());
		_layout.setGeometry(geometry().atOrigin().shrink({10, 10}));
	}

public:
	LinearLayoutDemo(const std::string &p_name, spk::Widget *p_parent) :
		spk::Widget(p_name, p_parent),
		_background(p_name + "/Background", this),
		_fixedButton(p_name + "/Fixed", "Fixed 96", this),
		_minimumButton(p_name + "/Minimum", "Minimum", this),
		_extendButton(p_name + "/Extend", "Extend", this),
		_horizontalExtendButton(p_name + "/HorizontalExtend", "Horizontal", this)
	{
		_layout.setElementPadding({8, 8});

		auto *fixed = _layout.addWidget(&_fixedButton, spk::Layout::SizePolicy::Fixed);
		fixed->setSize({96, 42});
		_layout.addWidget(&_minimumButton, spk::Layout::SizePolicy::Minimum);
		_layout.addWidget(&_extendButton, spk::Layout::SizePolicy::Extend);
		_layout.addWidget(&_horizontalExtendButton, spk::Layout::SizePolicy::HorizontalExtend);

		_background.activate();
		_fixedButton.activate();
		_minimumButton.activate();
		_extendButton.activate();
		_horizontalExtendButton.activate();

		setMinimalSize({480, 86});
	}
};

class GridLayoutDemo : public spk::Widget
{
private:
	spk::Panel _background;
	spk::GridLayout _layout;
	spk::PushButton _topLeft;
	spk::PushButton _topRight;
	spk::PushButton _middle;
	spk::PushButton _bottomLeft;
	spk::PushButton _bottomRight;

	void _onGeometryChange() override
	{
		_background.setGeometry(geometry().atOrigin());
		_layout.setGeometry(geometry().atOrigin().shrink({10, 10}));
	}

public:
	GridLayoutDemo(const std::string &p_name, spk::Widget *p_parent) :
		spk::Widget(p_name, p_parent),
		_background(p_name + "/Background", this),
		_topLeft(p_name + "/TopLeft", "0,0", this),
		_topRight(p_name + "/TopRight", "2,0", this),
		_middle(p_name + "/Middle", "extend center", this),
		_bottomLeft(p_name + "/BottomLeft", "0,2", this),
		_bottomRight(p_name + "/BottomRight", "2,2", this)
	{
		_layout.setElementPadding({6, 6});
		_layout.setWidget(0, 0, &_topLeft, spk::Layout::SizePolicy::Minimum);
		_layout.setWidget(2, 0, &_topRight, spk::Layout::SizePolicy::Minimum);
		_layout.setWidget(1, 1, &_middle, spk::Layout::SizePolicy::Extend);
		_layout.setWidget(0, 2, &_bottomLeft, spk::Layout::SizePolicy::Minimum);
		_layout.setWidget(2, 2, &_bottomRight, spk::Layout::SizePolicy::Minimum);

		_background.activate();
		_topLeft.activate();
		_topRight.activate();
		_middle.activate();
		_bottomLeft.activate();
		_bottomRight.activate();

		setMinimalSize({480, 170});
	}
};

class FormLayoutDemo : public spk::Widget
{
private:
	spk::Panel _background;
	spk::FormLayout _form;
	spk::TextLabel _nameLabel;
	spk::TextEdit _nameEdit;
	spk::TextLabel _amountLabel;
	spk::IntSpinBox _amountSpinBox;
	spk::TextLabel _volumeLabel;
	spk::SliderBar _volumeSlider;
	spk::TextLabel _enabledLabel;
	spk::CheckableIconButton _enabledButton;

	void _onGeometryChange() override
	{
		_background.setGeometry(geometry().atOrigin());
		_form.setGeometry(geometry().atOrigin().shrink({10, 10}));
	}

public:
	FormLayoutDemo(const std::string &p_name, spk::Widget *p_parent) :
		spk::Widget(p_name, p_parent),
		_background(p_name + "/Background", this),
		_nameLabel(p_name + "/NameLabel", "Name", this),
		_nameEdit(p_name + "/NameEdit", this),
		_amountLabel(p_name + "/AmountLabel", "Amount", this),
		_amountSpinBox(p_name + "/AmountSpinBox", this),
		_volumeLabel(p_name + "/VolumeLabel", "Volume", this),
		_volumeSlider(p_name + "/VolumeSlider", this),
		_enabledLabel(p_name + "/EnabledLabel", "Enabled", this),
		_enabledButton(p_name + "/EnabledButton", 8, 9, this)
	{
		_nameEdit.setText("Sparkle");
		_amountSpinBox.setValue(4);
		_volumeSlider.setValue(65.0f);
		_enabledButton.setChecked(true);

		_form.setElementPadding({8, 8});
		_form.addRow(&_nameLabel, &_nameEdit);
		_form.addRow(&_amountLabel, &_amountSpinBox);
		_form.addRow(&_volumeLabel, &_volumeSlider);
		_form.addRow(&_enabledLabel, &_enabledButton, spk::Layout::SizePolicy::Minimum, spk::Layout::SizePolicy::Minimum);

		_background.activate();
		_nameLabel.activate();
		_nameEdit.activate();
		_amountLabel.activate();
		_amountSpinBox.activate();
		_volumeLabel.activate();
		_volumeSlider.activate();
		_enabledLabel.activate();
		_enabledButton.activate();

		setMinimalSize({480, 190});
	}
};

class ContainerViewportDemo : public spk::Widget
{
private:
	spk::Panel _background;
	spk::ContainerWidget _container;
	spk::Panel _content;
	spk::TextLabel _contentLabel;

	void _onGeometryChange() override
	{
		_background.setGeometry(geometry().atOrigin());
		_container.setGeometry(geometry().atOrigin().shrink({12, 12}));
		_contentLabel.setGeometry(spk::Rect2D(20, 20, 430, 40));
	}

public:
	ContainerViewportDemo(const std::string &p_name, spk::Widget *p_parent) :
		spk::Widget(p_name, p_parent),
		_background(p_name + "/Background", this),
		_container(p_name + "/Container", this),
		_content(p_name + "/Content", &_container),
		_contentLabel(p_name + "/ContentLabel", "Content is larger than the viewport and offset inside the container.", &_content)
	{
		_container.setContent(&_content);
		_container.setContentSize({560, 160});
		_container.setContentAnchor({-70, -20});

		_background.activate();
		_container.activate();
		_content.activate();
		_contentLabel.activate();

		setMinimalSize({480, 130});
	}
};

class ScalablePanelDemo : public spk::ScalableWidget
{
private:
	spk::Panel _background;
	spk::TextLabel _label;

	void _onGeometryChange() override
	{
		_background.setGeometry(geometry().atOrigin());
		_label.setGeometry(geometry().atOrigin());
	}

public:
	ScalablePanelDemo(const std::string &p_name, spk::Widget *p_parent) :
		spk::ScalableWidget(p_name, p_parent),
		_background(p_name + "/Background", this),
		_label(p_name + "/Label", "Drag an edge to resize this scalable widget", this)
	{
		_background.activate();
		_label.activate();
		setMinimalSize({300, 86});
	}
};

class OverviewPage : public ShowcaseGridPage
{
public:
	OverviewPage(const std::string &p_name, spk::Widget *p_parent) :
		ShowcaseGridPage(p_name, p_parent)
	{
		addNoteRow(
			"Existing widgets",
			"Pages in this showcase are built from Sparkle widgets that already exist: labels, text areas, text edits, panels, push buttons, icon buttons, checkable icon buttons, command panels, image labels, animation labels, sliders, scroll bars, scroll areas, spin boxes, menu bars, prompt panels, message boxes, interface windows, layouts, container widgets, scalable widgets, screens, and debug overlays.");
		addNoteRow(
			"Composition",
			"The showcase shell uses a header, a navigation panel, and a page registry. Each page is a regular widget composed through HorizontalLayout, VerticalLayout, GridLayout, or FormLayout.");
		addNoteRow(
			"Next useful work",
			"The planned widgets page keeps missing primitives visible: tooltips, popup overlay host, tabs, combo boxes, checkbox/radio/toggle controls, progress bars, modal stack behavior, focus traversal, and clipboard-aware text selection.");

		finishPage();
	}

	std::string pageName() const override
	{
		return "Overview";
	}
};

class TextAlignmentGridDemo : public spk::Widget
{
private:
	spk::GridLayout _grid;
	std::vector<std::unique_ptr<Cell<spk::TextLabel>>> _cells;

	void _onGeometryChange() override
	{
		_grid.setGeometry(geometry().atOrigin());
	}

	void _addAlignmentCell(
		size_t p_column,
		size_t p_row,
		spk::HorizontalAlignment p_horizontalAlignment,
		spk::VerticalAlignment p_verticalAlignment,
		std::string_view p_caption)
	{
		auto cell = std::make_unique<Cell<spk::TextLabel>>(
			name() + "/Alignment" + std::to_string(p_column) + "_" + std::to_string(p_row),
			this,
			p_caption);
		spk::TextLabel &label = cell->content();

		label.setAlignment(p_horizontalAlignment, p_verticalAlignment);
		label.setPadding({8, 6});
		label.setMinimalSize({150, 64});

		_grid.setWidget(p_column, p_row, cell.get(), spk::Layout::SizePolicy::Minimum);
		_cells.push_back(std::move(cell));
	}

public:
	TextAlignmentGridDemo(const std::string &p_name, spk::Widget *p_parent) :
		spk::Widget(p_name, p_parent)
	{
		_grid.setElementPadding({6, 6});

		_addAlignmentCell(0, 0, spk::HorizontalAlignment::Left, spk::VerticalAlignment::Top, "Left Top");
		_addAlignmentCell(1, 0, spk::HorizontalAlignment::Centered, spk::VerticalAlignment::Top, "Center Top");
		_addAlignmentCell(2, 0, spk::HorizontalAlignment::Right, spk::VerticalAlignment::Top, "Right Top");

		_addAlignmentCell(0, 1, spk::HorizontalAlignment::Left, spk::VerticalAlignment::Centered, "Left Center");
		_addAlignmentCell(1, 1, spk::HorizontalAlignment::Centered, spk::VerticalAlignment::Centered, "Center");
		_addAlignmentCell(2, 1, spk::HorizontalAlignment::Right, spk::VerticalAlignment::Centered, "Right Center");

		_addAlignmentCell(0, 2, spk::HorizontalAlignment::Left, spk::VerticalAlignment::Down, "Left Down");
		_addAlignmentCell(1, 2, spk::HorizontalAlignment::Centered, spk::VerticalAlignment::Down, "Center Down");
		_addAlignmentCell(2, 2, spk::HorizontalAlignment::Right, spk::VerticalAlignment::Down, "Right Down");

		setMinimalSize(_grid.minimalSize());
		activate();
	}
};

class TextWidgetsPage : public ShowcaseGridPage
{
private:
	spk::DynamicTextLabel *_dynamicLabel = nullptr;

public:
	TextWidgetsPage(const std::string &p_name, spk::Widget *p_parent) :
		ShowcaseGridPage(p_name, p_parent)
	{
		auto &label = makeWidget<spk::TextLabel>("Label", "TextLabel: centered by default");
		addRow("TextLabel", label);

		auto &alignmentGrid = makeWidget<TextAlignmentGridDemo>("AlignmentGrid");
		addRow("Alignment", alignmentGrid);

		auto &area = makeWidget<spk::TextArea>("TextArea");
		area.setText("TextArea wraps longer content inside its allocated width.\nExplicit line breaks are also preserved.");
		area.setLinePadding(4);
		area.setMinimalSize(LargeControlSize);
		addRow("TextArea", area);

		_dynamicLabel = &makeWidget<spk::DynamicTextLabel>("DynamicLabel");
		_dynamicLabel->setTextProducer([]() -> std::string {
			return "Program time: " + spk::TimeUtils::getTime().toString();
		});
		addRow("DynamicTextLabel", *_dynamicLabel);

		finishPage();
	}

	std::string pageName() const override
	{
		return "Labels / Text";
	}
};

class InputWidgetsPage : public ShowcaseGridPage
{
public:
	InputWidgetsPage(const std::string &p_name, spk::Widget *p_parent) :
		ShowcaseGridPage(p_name, p_parent)
	{
		auto &emptyEdit = makeWidget<spk::TextEdit>("PlaceholderEdit");
		emptyEdit.setPlaceholder("Type text here");
		addRow("TextEdit", emptyEdit);

		auto &filledEdit = makeWidget<spk::TextEdit>("FilledEdit");
		filledEdit.setText("Editable text");
		addRow("Filled edit", filledEdit);

		auto &passwordEdit = makeWidget<spk::TextEdit>("PasswordEdit");
		passwordEdit.setText("secret");
		passwordEdit.setObscured(true);
		addRow("Obscured edit", passwordEdit);

		auto &spinBox = makeWidget<spk::SpinBox<int>>("SpinBox");
		spinBox.setValue(8);
		spinBox.setStep(2);
		spinBox.setLimits(0, 20);
		addRow("SpinBox<int>", spinBox);

		auto &intSpinBox = makeWidget<spk::IntSpinBox>("IntSpinBox");
		intSpinBox.setValue(42);
		intSpinBox.setStep(5);
		addRow("Numeric int", intSpinBox);

		auto &floatSpinBox = makeWidget<spk::FloatSpinBox>("FloatSpinBox");
		floatSpinBox.setValue(3.14f);
		floatSpinBox.setStep(0.25f);
		addRow("Numeric float", floatSpinBox);

		finishPage();
	}

	std::string pageName() const override
	{
		return "Input";
	}
};

class ButtonsCommandsPage : public ShowcaseGridPage
{
private:
	int _clickCount = 0;
	spk::PushButton::Contract _clickContract;
	spk::CommandPanel::Contract _applyContract;
	spk::CommandPanel::Contract _resetContract;
	spk::CheckableIconButton::StateContract _stateContract;
	spk::TextLabel *_clickStatus = nullptr;
	spk::TextLabel *_checkStatus = nullptr;

public:
	ButtonsCommandsPage(const std::string &p_name, spk::Widget *p_parent) :
		ShowcaseGridPage(p_name, p_parent)
	{
		auto &pushButton = makeWidget<spk::PushButton>("PushButton", "Click me");
		addRow("PushButton", pushButton);

		_clickStatus = &makeWidget<spk::TextLabel>("ClickStatus", "Clicks: 0");
		addRow("Click state", *_clickStatus);

		_clickContract = pushButton.subscribeToClick([this]() {
			++_clickCount;
			_clickStatus->setText("Clicks: " + std::to_string(_clickCount));
		});

		auto &iconButton = makeWidget<spk::IconButton>("IconButton");
		iconButton.setIconSpriteID(3);
		iconButton.setMinimalSize({42, 42});
		addRow("IconButton", iconButton, spk::Layout::SizePolicy::Minimum);

		auto &checkableButton = makeWidget<spk::CheckableIconButton>("CheckableIconButton");
		checkableButton.setMinimalSize({42, 42});
		addRow("Checkable icon", checkableButton, spk::Layout::SizePolicy::Minimum);

		_checkStatus = &makeWidget<spk::TextLabel>("CheckStatus", "Checked: false");
		addRow("Check state", *_checkStatus);

		_stateContract = checkableButton.subscribeToState([this](const bool &p_state) {
			_checkStatus->setText("Checked: " + boolText(p_state));
		});

		auto &commandPanel = makeWidget<spk::CommandPanel>("CommandPanel");
		commandPanel.addButton("apply", "Apply");
		commandPanel.addButton("reset", "Reset");
		commandPanel.addButton("close", "Close");
		commandPanel.setElementPadding({6, 6});
		addRow("CommandPanel", commandPanel);

		_applyContract = commandPanel.subscribe("apply", [this]() {
			_clickStatus->setText("CommandPanel: apply");
		});
		_resetContract = commandPanel.subscribe("reset", [this]() {
			_clickCount = 0;
			_clickStatus->setText("Clicks: 0");
		});

		finishPage();
	}

	std::string pageName() const override
	{
		return "Buttons / Commands";
	}
};

class PanelsMediaPage : public ShowcaseGridPage
{
public:
	PanelsMediaPage(const std::string &p_name, spk::Widget *p_parent) :
		ShowcaseGridPage(p_name, p_parent)
	{
		auto &defaultPanel = makeWidget<spk::Panel>("DefaultPanel");
		defaultPanel.setMinimalSize({360, 58});
		addRow("Panel default", defaultPanel);

		auto &lightPanel = makeWidget<spk::Panel>(
			"LightPanel",
			spk::WidgetStyle::Collection::style(spk::WidgetStyle::Collection::DefaultLight));
		lightPanel.setMinimalSize({360, 58});
		addRow("Panel light", lightPanel);

		auto &darkPanel = makeWidget<spk::Panel>(
			"DarkPanel",
			spk::WidgetStyle::Collection::style(spk::WidgetStyle::Collection::DefaultDark));
		darkPanel.setMinimalSize({360, 58});
		addRow("Panel dark", darkPanel);

		const auto &iconset = spk::WidgetStyle::Collection::style(spk::WidgetStyle::Collection::Default).iconSpriteSheet();

		auto &imageLabel = makeWidget<spk::ImageLabel>("ImageLabel");
		imageLabel.setTexture(iconset);
		imageLabel.setSection(iconset->sprite(3));
		imageLabel.setMinimalSize({76, 76});
		addRow("ImageLabel", imageLabel, spk::Layout::SizePolicy::Minimum);

		auto &animationLabel = makeWidget<spk::AnimationLabel>("AnimationLabel");
		animationLabel.setSpriteSheet(iconset);
		animationLabel.setAnimationRange(0, 9);
		animationLabel.setLoopSpeed(spk::Duration(140.0L, spk::TimeUnit::Millisecond));
		animationLabel.setMinimalSize({76, 76});
		addRow("AnimationLabel", animationLabel, spk::Layout::SizePolicy::Minimum);

		finishPage();
	}

	std::string pageName() const override
	{
		return "Panels / Media";
	}
};

class SlidersScrollPage : public ShowcaseGridPage
{
private:
	spk::SliderBar::Contract _horizontalContract;
	spk::SliderBar::Contract _verticalContract;
	spk::ScrollBar::Contract _scrollContract;
	spk::TextLabel *_sliderValueLabel = nullptr;

public:
	SlidersScrollPage(const std::string &p_name, spk::Widget *p_parent) :
		ShowcaseGridPage(p_name, p_parent)
	{
		auto &horizontalSlider = makeWidget<spk::SliderBar>("HorizontalSlider");
		horizontalSlider.setRange(0.0f, 100.0f);
		horizontalSlider.setValue(35.0f);
		horizontalSlider.setMinimalSize({360, 30});
		addRow("Horizontal slider", horizontalSlider);

		auto &verticalSlider = makeWidget<spk::SliderBar>("VerticalSlider", spk::Orientation::Vertical);
		verticalSlider.setRange(0.0f, 100.0f);
		verticalSlider.setValue(65.0f);
		verticalSlider.setScale(0.25f);
		verticalSlider.setMinimalSize({34, 160});
		addRow("Vertical slider", verticalSlider, spk::Layout::SizePolicy::Minimum);

		_sliderValueLabel = &makeWidget<spk::TextLabel>("SliderValue", "Slider ratio: 0.35");
		addRow("Live state", *_sliderValueLabel);

		_horizontalContract = horizontalSlider.subscribeToEdition([this](float p_ratio) {
			_sliderValueLabel->setText("Horizontal slider ratio: " + std::to_string(p_ratio));
		});
		_verticalContract = verticalSlider.subscribeToEdition([this](float p_ratio) {
			_sliderValueLabel->setText("Vertical slider ratio: " + std::to_string(p_ratio));
		});

		auto &scrollBar = makeWidget<spk::ScrollBar>("ScrollBar");
		scrollBar.setScale(0.28f);
		scrollBar.setRatio(0.25f);
		scrollBar.setMinimalSize({360, 34});
		addRow("ScrollBar", scrollBar);
		_scrollContract = scrollBar.subscribeToEdition([this](float p_ratio) {
			_sliderValueLabel->setText("ScrollBar ratio: " + std::to_string(p_ratio));
		});

		auto &scrollArea = makeWidget<spk::ScrollArea<ScrollableDemoContent>>("ScrollArea");
		scrollArea.setContentSize({620, 330});
		scrollArea.setMinimalSize({480, 220});
		addRow("ScrollArea", scrollArea);

		finishPage();
	}

	std::string pageName() const override
	{
		return "Sliders / Scroll";
	}
};

class LayoutWidgetsPage : public ShowcaseGridPage
{
public:
	LayoutWidgetsPage(const std::string &p_name, spk::Widget *p_parent) :
		ShowcaseGridPage(p_name, p_parent)
	{
		auto &linearDemo = makeWidget<LinearLayoutDemo>("LinearLayoutDemo");
		addRow("LinearLayout", linearDemo);

		auto &gridDemo = makeWidget<GridLayoutDemo>("GridLayoutDemo");
		addRow("GridLayout", gridDemo);

		auto &formDemo = makeWidget<FormLayoutDemo>("FormLayoutDemo");
		addRow("FormLayout", formDemo);

		addNoteRow(
			"Size policies",
			"These demos intentionally mix Fixed, Minimum, Extend, HorizontalExtend, and form label/field policies. Resize the showcase window to see recomputation.");

		finishPage();
	}

	std::string pageName() const override
	{
		return "Layouts";
	}
};

class ShellWidgetsPage : public ShowcaseGridPage
{
public:
	ShellWidgetsPage(const std::string &p_name, spk::Widget *p_parent) :
		ShowcaseGridPage(p_name, p_parent)
	{
		auto &menuBar = makeWidget<spk::MenuBar>("MenuBar");
		menuBar.setHeight(30);
		menuBar.setMinimalSize({520, 120});
		auto *fileMenu = menuBar.addMenu("File");
		fileMenu->addItem("New", []() {
		});
		fileMenu->addItem("Open", []() {
		});
		fileMenu->addBreak();
		fileMenu->addItem("Exit", []() {
		});
		auto *editMenu = menuBar.addMenu("Edit");
		editMenu->addItem("Undo", []() {
		});
		editMenu->addItem("Redo", []() {
		});
		menuBar.addMenu("Empty");
		addRow("MenuBar", menuBar);

		auto &workspace = makeWidget<spk::Workspace<spk::Panel>>("Workspace");
		workspace.menuBar().addMenu("Scene")->addItem("Refresh", []() {
		});
		workspace.menuBar().addMenu("View")->addItem("Grid", []() {
		});
		workspace.setMinimalSize({520, 150});
		addRow("Workspace", workspace);

		auto &window = makeWidget<spk::InterfaceWindow<spk::Panel>>("InterfaceWindow");
		window.setTitle("InterfaceWindow<Panel>");
		window.setContentPadding({8, 8, 8, 8});
		window.setMinimalSize({520, 170});
		addRow("InterfaceWindow", window);

		finishPage();
	}

	std::string pageName() const override
	{
		return "Menu / Workspace";
	}
};

class DialogsPage : public ShowcaseGridPage
{
public:
	DialogsPage(const std::string &p_name, spk::Widget *p_parent) :
		ShowcaseGridPage(p_name, p_parent)
	{
		auto &prompt = makeWidget<spk::PromptPanel>("PromptPanel");
		prompt.setMessage("PromptPanel composes a TextArea and a CommandPanel.");
		prompt.addButton("accept", "Accept");
		prompt.addButton("cancel", "Cancel");
		prompt.setButtonPadding({6, 6});
		prompt.setMinimalSize({520, 150});
		addRow("PromptPanel", prompt);

		auto &infoBox = makeWidget<spk::InformationMessageBox>("InformationMessageBox");
		infoBox.setTitle("Information");
		infoBox.setText("MessageBox variants are interface-window based dialog widgets.");
		infoBox.setMinimalSize({520, 180});
		addRow("Information box", infoBox);

		auto &requestBox = makeWidget<spk::RequestMessageBox>("RequestMessageBox");
		requestBox.setTitle("Request");
		requestBox.setText("RequestMessageBox exposes two configurable actions.");
		requestBox.configure("Yes", []() {
		},
							 "No",
							 []() {
							 });
		requestBox.setMinimalSize({520, 180});
		addRow("Request box", requestBox);

		finishPage();
	}

	std::string pageName() const override
	{
		return "Dialogs";
	}
};

class DiagnosticsPage : public ShowcaseGridPage
{
public:
	DiagnosticsPage(const std::string &p_name, spk::Widget *p_parent) :
		ShowcaseGridPage(p_name, p_parent)
	{
		auto &debugOverlay = makeWidget<spk::DebugOverlay>("DebugOverlay");
		debugOverlay.configureRows(3, 2);
		debugOverlay.setText(0, 0, "Active page");
		debugOverlay.setText(0, 1, "Diagnostics");
		debugOverlay.setText(1, 0, "Keyboard focus");
		debugOverlay.setText(1, 1, "click a text edit");
		debugOverlay.setText(2, 0, "Geometry");
		debugOverlay.setText(2, 1, "live widget tree");
		debugOverlay.setMinimalSize({480, 96});
		addRow("DebugOverlay", debugOverlay);

		auto &containerDemo = makeWidget<ContainerViewportDemo>("ContainerViewportDemo");
		addRow("ContainerWidget", containerDemo);

		auto &scalableDemo = makeWidget<ScalablePanelDemo>("ScalablePanelDemo");
		addRow("ScalableWidget", scalableDemo);

		addNoteRow(
			"Runtime state",
			"Keyboard focus: " +
				std::string(spk::Widget::focusedWidget(spk::Widget::FocusType::Keyboard) != nullptr ? "present" : "none") +
				"\nMouse focus: " +
				std::string(spk::Widget::focusedWidget(spk::Widget::FocusType::Mouse) != nullptr ? "present" : "none") +
				"\nThis page validates debug overlays, container clipping, and scalable edge resizing. Screen is listed on Overview because it is a non-visual application-structure widget.");

		finishPage();
	}

	std::string pageName() const override
	{
		return "Diagnostics";
	}
};

class PlannedWidgetsPage : public ShowcaseGridPage
{
public:
	PlannedWidgetsPage(const std::string &p_name, spk::Widget *p_parent) :
		ShowcaseGridPage(p_name, p_parent)
	{
		addNoteRow(
			"Overlay layer",
			"Missing: generic PopupLayer or OverlayHost. Needed for tooltips, dropdowns, context menus, and true modal backdrops.");
		addNoteRow(
			"Selection widgets",
			"Missing: TabBar/TabView, ListView, ComboBox, CheckBox, RadioButton/RadioGroup, ToggleSwitch.");
		addNoteRow(
			"Feedback widgets",
			"Missing: ProgressBar, indeterminate progress, semantic status styles, and richer disabled/focus/hover states.");
		addNoteRow(
			"Text editing",
			"Still to showcase after implementation: selection range, clipboard, Ctrl+A/C/X/V, Shift+arrows, double-click word selection, and Tab focus traversal.");

		finishPage();
	}

	std::string pageName() const override
	{
		return "Planned Widgets";
	}
};

class MainWidget : public spk::Widget
{
private:
	spk::VerticalLayout _vLayout;

	Header _header;
	Body _body;

	void _onGeometryChange() override
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

		_body.addPage<OverviewPage>("OverviewPage");
		_body.addPage<TextWidgetsPage>("TextWidgetsPage");
		_body.addPage<InputWidgetsPage>("InputWidgetsPage");
		_body.addPage<ButtonsCommandsPage>("ButtonsCommandsPage");
		_body.addPage<PanelsMediaPage>("PanelsMediaPage");
		_body.addPage<SlidersScrollPage>("SlidersScrollPage");
		_body.addPage<LayoutWidgetsPage>("LayoutWidgetsPage");
		_body.addPage<ShellWidgetsPage>("ShellWidgetsPage");
		_body.addPage<DialogsPage>("DialogsPage");
		_body.addPage<DiagnosticsPage>("DiagnosticsPage");
		_body.addPage<PlannedWidgetsPage>("PlannedWidgetsPage");

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
