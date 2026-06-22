#include "structures/widget/spk_interface_window.hpp"

#include <algorithm>
#include <limits>
#include <stdexcept>

#include "structures/widget/spk_widget_style.hpp"

namespace
{
	constexpr size_t CloseIconID = 0;
	constexpr size_t MaximizeIconID = 1;
	constexpr size_t RestoreIconID = 2;
	constexpr size_t MinimizeIconID = 3;
	constexpr spk::Vector2UInt TitleButtonIconSize = {12, 12};
	constexpr spk::Vector2UInt TitleButtonIconPadding = {4, 4};
	constexpr spk::Vector2Int TitleButtonCornerSize = {2, 2};

	[[nodiscard]] std::shared_ptr<spk::SpriteSheet> defaultIconset()
	{
		return spk::WidgetStyle::Collection::style(spk::WidgetStyle::Collection::Default).iconSpriteSheet();
	}

	[[nodiscard]] uint32_t safeAdd(uint32_t p_left, uint32_t p_right)
	{
		return (p_left > std::numeric_limits<uint32_t>::max() - p_right)
				   ? std::numeric_limits<uint32_t>::max()
				   : p_left + p_right;
	}
}

namespace spk
{
	IInterfaceWindow::MenuBar::MenuBar(const std::string &p_name, spk::Widget *p_parent) :
		spk::Widget(p_name, p_parent),
		_titleLabel(p_name + "::titleLabel", this),
		_minimizeButton(p_name + "::minimizeButton", this),
		_maximizeButton(p_name + "::maximizeButton", this),
		_closeButton(p_name + "::closeButton", this)
	{
		_titleLabel.useStyle(spk::WidgetStyle::Collection::style(spk::WidgetStyle::Collection::DefaultInterfaceWindowTitle));
		_titleLabel.setAlignment(spk::HorizontalAlignment::Left, spk::VerticalAlignment::Centered);

		_minimizeButton.setIcon(defaultIconset(), MinimizeIconID);
		_maximizeButton.setIcon(defaultIconset(), MaximizeIconID);
		_closeButton.setIcon(defaultIconset(), CloseIconID);
		_minimizeButton.setIconSize(TitleButtonIconSize);
		_maximizeButton.setIconSize(TitleButtonIconSize);
		_closeButton.setIconSize(TitleButtonIconSize);

		_minimizeButton.setIconPadding(TitleButtonIconPadding);
		_maximizeButton.setIconPadding(TitleButtonIconPadding);
		_closeButton.setIconPadding(TitleButtonIconPadding);
		_minimizeButton.releasedBackground().setCornerSize(TitleButtonCornerSize);
		_minimizeButton.pressedBackground().setCornerSize(TitleButtonCornerSize);
		_maximizeButton.releasedBackground().setCornerSize(TitleButtonCornerSize);
		_maximizeButton.pressedBackground().setCornerSize(TitleButtonCornerSize);
		_closeButton.releasedBackground().setCornerSize(TitleButtonCornerSize);
		_closeButton.pressedBackground().setCornerSize(TitleButtonCornerSize);

		sizeHint().configureMinimalGenerator([this]() {
			const spk::Vector2UInt titleSize = _titleLabel.minimalSize();
			const unsigned int buttonSize = _minimumControlButtonSize();

			return spk::Vector2UInt(
				titleSize.x + buttonSize * 3 + static_cast<unsigned int>(Margin) * 5,
				std::max(titleSize.y, buttonSize + static_cast<unsigned int>(Margin) * 2));
		});

		activate();
	}

	unsigned int IInterfaceWindow::MenuBar::_minimumControlButtonSize() const
	{
		return std::max({1u, _minimizeButton.minimalSize().x, _minimizeButton.minimalSize().y, _maximizeButton.minimalSize().x, _maximizeButton.minimalSize().y, _closeButton.minimalSize().x, _closeButton.minimalSize().y});
	}

	unsigned int IInterfaceWindow::MenuBar::_controlButtonSize() const
	{
		const unsigned int height = geometry().height();
		const unsigned int availableSize =
			(height > static_cast<unsigned int>(Margin * 2)) ? height - Margin * 2 : height;
		return std::max(_minimumControlButtonSize(), availableSize);
	}

	void IInterfaceWindow::MenuBar::_onGeometryChange()
	{
		const unsigned int buttonSize = _controlButtonSize();

		const int activeCount =
			(_minimizeButton.isActivated() ? 1 : 0) +
			(_maximizeButton.isActivated() ? 1 : 0) +
			(_closeButton.isActivated() ? 1 : 0);

		const int usedWidth = activeCount * static_cast<int>(buttonSize) + (activeCount + 1) * Margin;
		const int titleWidth = std::max(0, static_cast<int>(geometry().width()) - usedWidth - Margin);

		_titleLabel.setGeometry(spk::Rect2D(Margin, Margin, static_cast<unsigned int>(titleWidth), buttonSize));

		int anchorX = Margin + titleWidth + Margin;
		const auto placeButton = [&](spk::PushButton &p_button) {
			if (p_button.isActivated() == false)
			{
				return;
			}
			p_button.setGeometry(spk::Rect2D(anchorX, Margin, buttonSize, buttonSize));
			anchorX += static_cast<int>(buttonSize) + Margin;
		};

		placeButton(_minimizeButton);
		placeButton(_maximizeButton);
		placeButton(_closeButton);
	}

	void IInterfaceWindow::MenuBar::_activateMenuButton(Button p_button)
	{
		switch (p_button)
		{
		case Button::Minimize:
			_minimizeButton.activate();
			break;
		case Button::Maximize:
			_maximizeButton.activate();
			break;
		case Button::Close:
			_closeButton.activate();
			break;
		}
		_onGeometryChange();
	}

	void IInterfaceWindow::MenuBar::_deactivateMenuButton(Button p_button)
	{
		switch (p_button)
		{
		case Button::Minimize:
			_minimizeButton.deactivate();
			break;
		case Button::Maximize:
			_maximizeButton.deactivate();
			break;
		case Button::Close:
			_closeButton.deactivate();
			break;
		}
		_onGeometryChange();
	}

	spk::TextLabel &IInterfaceWindow::MenuBar::titleLabel()
	{
		return _titleLabel;
	}

	const spk::TextLabel &IInterfaceWindow::MenuBar::titleLabel() const
	{
		return _titleLabel;
	}

	spk::PushButton &IInterfaceWindow::MenuBar::minimizeButton()
	{
		return _minimizeButton;
	}

	const spk::PushButton &IInterfaceWindow::MenuBar::minimizeButton() const
	{
		return _minimizeButton;
	}

	spk::PushButton &IInterfaceWindow::MenuBar::maximizeButton()
	{
		return _maximizeButton;
	}

	const spk::PushButton &IInterfaceWindow::MenuBar::maximizeButton() const
	{
		return _maximizeButton;
	}

	spk::PushButton &IInterfaceWindow::MenuBar::closeButton()
	{
		return _closeButton;
	}

	const spk::PushButton &IInterfaceWindow::MenuBar::closeButton() const
	{
		return _closeButton;
	}

	IInterfaceWindow::IInterfaceWindow(const std::string &p_name, spk::Widget *p_parent) :
		spk::ScalableWidget(p_name, p_parent),
		_backgroundFrame(
			p_name + "::backgroundFrame",
			spk::WidgetStyle::Collection::style(spk::WidgetStyle::Collection::DefaultDark),
			this),
		_minimizedBackgroundFrame(
			p_name + "::minimizedBackgroundFrame",
			spk::WidgetStyle::Collection::style(spk::WidgetStyle::Collection::DefaultDark),
			this),
		_menuBar(p_name + "::menuBar", this)
	{
		_minimizedBackgroundFrame.deactivate();
		_menuBar.titleLabel().setText(p_name);

		_minimizeContract = _menuBar._minimizeButton.subscribeToClick([this]() {
			minimize();
		});

		_maximizeContract = _menuBar._maximizeButton.subscribeToClick([this]() {
			maximize();
		});

		activate();
	}

	IInterfaceWindow::ResizeContract IInterfaceWindow::subscribeOnResize(ResizeCallback p_callback)
	{
		return _onResizeProvider.subscribe(std::move(p_callback));
	}

	IInterfaceWindow::EventContract IInterfaceWindow::subscribeTo(Event p_event, EventCallback p_callback)
	{
		switch (p_event)
		{
		case Event::Minimize:
			return _menuBar._minimizeButton.subscribeToClick(std::move(p_callback));
		case Event::Maximize:
			return _menuBar._maximizeButton.subscribeToClick(std::move(p_callback));
		case Event::Close:
			return _menuBar._closeButton.subscribeToClick(std::move(p_callback));
		}

		throw std::invalid_argument("Unknown interface window event");
	}

	spk::Vector2UInt IInterfaceWindow::contentSize() const
	{
		const ContentPadding padding = _effectiveContentPadding();
		const uint32_t horizontalMargin = safeAdd(padding.left, padding.right);
		const uint32_t verticalMargin = safeAdd(_effectiveMenuHeight(), safeAdd(padding.top, padding.bottom));

		return {
			(geometry().width() > horizontalMargin) ? geometry().width() - horizontalMargin : 0,
			(geometry().height() > verticalMargin) ? geometry().height() - verticalMargin : 0};
	}

	unsigned int IInterfaceWindow::_effectiveMenuHeight() const
	{
		return std::max(_menuHeight, _menuBar.minimalSize().y);
	}

	IInterfaceWindow::ContentPadding IInterfaceWindow::_effectiveContentPadding() const
	{
		if (_contentPadding.has_value() == true)
		{
			return *_contentPadding;
		}

		const spk::Vector2Int cornerSize = _backgroundFrame.cornerSize();
		return ContentPadding{
			static_cast<uint32_t>(std::max(0, cornerSize.x + 2)),
			0,
			static_cast<uint32_t>(std::max(0, cornerSize.x + 2)),
			static_cast<uint32_t>(std::max(0, cornerSize.y + 2))};
	}

	void IInterfaceWindow::_onGeometryChange()
	{
		const unsigned int menuHeight = _effectiveMenuHeight();
		const spk::Vector2UInt menuSize = {geometry().width(), menuHeight};
		const spk::Vector2UInt frameSize = contentSize();
		const ContentPadding padding = _effectiveContentPadding();

		_onResizeProvider.trigger(frameSize);

		_backgroundFrame.setGeometry(geometry().atOrigin());
		_minimizedBackgroundFrame.setGeometry(spk::Rect2D({0, 0}, menuSize));
		_menuBar.setGeometry(spk::Rect2D({0, 0}, menuSize));

		if (_content != nullptr)
		{
			_content->setGeometry(spk::Rect2D({static_cast<int>(padding.left), static_cast<int>(menuHeight + padding.top)}, frameSize));
		}
	}

	void IInterfaceWindow::_onMouseMovedEvent(spk::MouseMovedEvent &p_event)
	{
		spk::ScalableWidget::_onMouseMovedEvent(p_event);

		if (p_event.isConsumed() == true || _isMoving == false)
		{
			return;
		}

		place(p_event.device().position - _moveOrigin);
		p_event.consume();
	}

	void IInterfaceWindow::_onMouseButtonPressedEvent(spk::MouseButtonPressedEvent &p_event)
	{
		spk::ScalableWidget::_onMouseButtonPressedEvent(p_event);

		if (p_event.isConsumed() == true)
		{
			return;
		}

		if (p_event->button == spk::Mouse::Left &&
			_menuBar.titleLabel().viewport().geometry().contains(p_event.device().position) == true)
		{
			_isMoving = true;
			_moveOrigin = p_event.device().position - geometry().anchor;
			p_event.device().requestCursor("Hand");
			takeFocus(FocusType::Mouse);
			p_event.consume();
			return;
		}

		if (absoluteGeometry().contains(p_event.device().position) == true)
		{
			p_event.consume();
		}
	}

	void IInterfaceWindow::_onMouseButtonReleasedEvent(spk::MouseButtonReleasedEvent &p_event)
	{
		spk::ScalableWidget::_onMouseButtonReleasedEvent(p_event);

		if (p_event->button != spk::Mouse::Left || _isMoving == false)
		{
			return;
		}

		_isMoving = false;
		releaseFocus(FocusType::Mouse);
		p_event.device().requestCursor("Arrow");
		p_event.consume();
	}

	IInterfaceWindow::MenuBar &IInterfaceWindow::menuBar()
	{
		return _menuBar;
	}

	const IInterfaceWindow::MenuBar &IInterfaceWindow::menuBar() const
	{
		return _menuBar;
	}

	spk::Panel &IInterfaceWindow::backgroundFrame()
	{
		return _backgroundFrame;
	}

	const spk::Panel &IInterfaceWindow::backgroundFrame() const
	{
		return _backgroundFrame;
	}

	spk::Panel &IInterfaceWindow::minimizedBackgroundFrame()
	{
		return _minimizedBackgroundFrame;
	}

	const spk::Panel &IInterfaceWindow::minimizedBackgroundFrame() const
	{
		return _minimizedBackgroundFrame;
	}

	void IInterfaceWindow::setContent(spk::Widget *p_content)
	{
		if (p_content != nullptr && p_content->parent() != &_backgroundFrame)
		{
			throw std::invalid_argument("IInterfaceWindow content must be a child of its background frame");
		}

		_content = p_content;

		if (_content != nullptr)
		{
			setMinimumContentSize(_content->minimalSize());
		}

		_onGeometryChange();
	}

	spk::Widget *IInterfaceWindow::content()
	{
		return _content;
	}

	const spk::Widget *IInterfaceWindow::content() const
	{
		return _content;
	}

	void IInterfaceWindow::setTitle(std::string_view p_title)
	{
		_menuBar.titleLabel().setText(p_title);
	}

	void IInterfaceWindow::setContentPadding(const ContentPadding &p_padding)
	{
		if (_contentPadding.has_value() == true && *_contentPadding == p_padding)
		{
			return;
		}

		_contentPadding = p_padding;

		if (_content != nullptr)
		{
			setMinimumContentSize(_content->minimalSize());
		}

		_onGeometryChange();
	}

	void IInterfaceWindow::resetContentPadding()
	{
		if (_contentPadding.has_value() == false)
		{
			return;
		}

		_contentPadding.reset();

		if (_content != nullptr)
		{
			setMinimumContentSize(_content->minimalSize());
		}

		_onGeometryChange();
	}

	void IInterfaceWindow::setMinimumContentSize(const spk::Vector2UInt &p_minimumContentSize)
	{
		const spk::Vector2UInt menuBarSize = _menuBar.minimalSize();
		const unsigned int menuHeight = _effectiveMenuHeight();
		const ContentPadding padding = _effectiveContentPadding();
		const uint32_t horizontalPadding = safeAdd(padding.left, padding.right);
		const uint32_t verticalPadding = safeAdd(padding.top, padding.bottom);

		const uint32_t width = std::max(safeAdd(p_minimumContentSize.x, horizontalPadding), menuBarSize.x);
		const uint32_t height = safeAdd(menuHeight, safeAdd(p_minimumContentSize.y, verticalPadding));

		// Re-applies the current geometry through setGeometry so the new minimum can clamp it;
		// setGeometry early-returns when nothing changes, which keeps resize callbacks from recursing.
		setMinimumSize({width, height});
		setGeometry(geometry());
	}

	void IInterfaceWindow::setMenuHeight(unsigned int p_menuHeight)
	{
		_menuHeight = p_menuHeight;

		if (_content != nullptr)
		{
			setMinimumContentSize(_content->minimalSize());
		}

		_onGeometryChange();
	}

	IInterfaceWindow::ContentPadding IInterfaceWindow::contentPadding() const
	{
		return _effectiveContentPadding();
	}

	unsigned int IInterfaceWindow::menuHeight() const
	{
		return _effectiveMenuHeight();
	}

	void IInterfaceWindow::minimize()
	{
		if (_isMaximized == true)
		{
			maximize();
		}

		if (_isMinimized == false)
		{
			_minimizedBackgroundFrame.activate();
			_backgroundFrame.deactivate();
			_isMinimized = true;
		}
		else
		{
			_minimizedBackgroundFrame.deactivate();
			_backgroundFrame.activate();
			_isMinimized = false;
		}
	}

	void IInterfaceWindow::maximize()
	{
		_backgroundFrame.activate();
		_minimizedBackgroundFrame.deactivate();
		_isMinimized = false;

		if (_isMaximized == false)
		{
			if (parent() == nullptr)
			{
				return;
			}

			_menuBar._maximizeButton.setIcon(defaultIconset(), RestoreIconID);
			_previousGeometry = geometry();
			setGeometry(spk::Rect2D({0, 0}, parent()->geometry().size));
			_isMaximized = true;
		}
		else
		{
			_menuBar._maximizeButton.setIcon(defaultIconset(), MaximizeIconID);
			setGeometry(_previousGeometry);
			_isMaximized = false;
		}
	}

	void IInterfaceWindow::close()
	{
		deactivate();
	}

	bool IInterfaceWindow::isMinimized() const
	{
		return _isMinimized;
	}

	bool IInterfaceWindow::isMaximized() const
	{
		return _isMaximized;
	}

	bool IInterfaceWindow::isMoving() const
	{
		return _isMoving;
	}

	void IInterfaceWindow::activateMenuButton(MenuBar::Button p_button)
	{
		_menuBar._activateMenuButton(p_button);
	}

	void IInterfaceWindow::deactivateMenuButton(MenuBar::Button p_button)
	{
		_menuBar._deactivateMenuButton(p_button);
	}
}
