#include "structures/widget/spk_message_box.hpp"

#include <algorithm>

namespace spk
{
	MessageBox::Content::Content(const std::string &p_name, spk::Widget *p_parent) :
		spk::Widget(p_name, p_parent),
		_textArea(p_name + "::textArea", this),
		_commandPanel(p_name + "::commandPanel", this)
	{
		_layout.setElementPadding({8, 8});

		_textArea.setBackgroundVisible(false);
		_textArea.setCornerSize({0, 0});

		_compose();

		sizeHint().configureMinimalGenerator([this]() {
			const spk::Vector2UInt commandPanelSize = _commandPanel.minimalSize();

			if (_textArea.text().empty() == true)
			{
				return commandPanelSize;
			}

			const spk::Vector2UInt textAreaSize = _textArea.computeMinimalSize(
				std::max(commandPanelSize.x, 200u));

			return spk::Vector2UInt(
				std::max(textAreaSize.x, commandPanelSize.x),
				textAreaSize.y + _layout.elementPadding().y + commandPanelSize.y);
		});

		activate();
	}

	void MessageBox::Content::_compose()
	{
		_layout.clear();
		_layout.addWidget(&_textArea, spk::Layout::SizePolicy::Extend);
		_layout.addWidget(&_commandPanel, spk::Layout::SizePolicy::Minimum);
	}

	void MessageBox::Content::_onGeometryChange()
	{
		_layout.setGeometry(geometry().atOrigin());
	}

	spk::VerticalLayout &MessageBox::Content::layout()
	{
		return _layout;
	}

	const spk::VerticalLayout &MessageBox::Content::layout() const
	{
		return _layout;
	}

	spk::TextArea &MessageBox::Content::textArea()
	{
		return _textArea;
	}

	const spk::TextArea &MessageBox::Content::textArea() const
	{
		return _textArea;
	}

	spk::CommandPanel &MessageBox::Content::commandPanel()
	{
		return _commandPanel;
	}

	const spk::CommandPanel &MessageBox::Content::commandPanel() const
	{
		return _commandPanel;
	}

	MessageBox::MessageBox(const std::string &p_name, spk::Widget *p_parent) :
		spk::IInterfaceWindow(p_name, p_parent),
		_content(p_name + "::content", &backgroundFrame())
	{
		setContent(&_content);

		_closeContract = subscribeTo(spk::IInterfaceWindow::Event::Close, [this]() {
			close();
		});

		_onResizeContract = subscribeOnResize(
			[this](const spk::Vector2UInt &p_availableSize) {
				const spk::Vector2UInt commandPanelMinimalSize = _content.commandPanel().minimalSize();
				const unsigned int textAreaWidth = std::max({commandPanelMinimalSize.x, p_availableSize.x, _minimalWidth});
				const spk::Vector2UInt textAreaMinimalSize = _content.textArea().computeMinimalSize(textAreaWidth);

				setMinimumContentSize(spk::Vector2UInt(std::max({textAreaMinimalSize.x, commandPanelMinimalSize.x, _minimalWidth}), textAreaMinimalSize.y + _content.layout().elementPadding().y + commandPanelMinimalSize.y));
			});
	}

	MessageBox::Content &MessageBox::content()
	{
		return _content;
	}

	const MessageBox::Content &MessageBox::content() const
	{
		return _content;
	}

	spk::TextArea &MessageBox::textArea()
	{
		return _content.textArea();
	}

	const spk::TextArea &MessageBox::textArea() const
	{
		return _content.textArea();
	}

	spk::CommandPanel &MessageBox::commandPanel()
	{
		return _content.commandPanel();
	}

	const spk::CommandPanel &MessageBox::commandPanel() const
	{
		return _content.commandPanel();
	}

	void MessageBox::setText(std::string_view p_text)
	{
		_content.textArea().setText(p_text);
		setMinimumContentSize(_content.minimalSize());
	}

	const spk::Font::Text &MessageBox::text() const
	{
		return _content.textArea().text();
	}

	spk::PushButton *MessageBox::addButton(const std::string &p_name, std::string_view p_label)
	{
		spk::PushButton *result = _content.commandPanel().addButton(p_name, p_label);
		setMinimumContentSize(_content.minimalSize());
		return result;
	}

	spk::PushButton *MessageBox::button(const std::string &p_name)
	{
		return _content.commandPanel().button(p_name);
	}

	const spk::PushButton *MessageBox::button(const std::string &p_name) const
	{
		return _content.commandPanel().button(p_name);
	}

	void MessageBox::removeButton(const std::string &p_name)
	{
		_content.commandPanel().removeButton(p_name);
	}

	spk::CommandPanel::Contract MessageBox::subscribe(const std::string &p_name, spk::CommandPanel::Callback p_callback)
	{
		return _content.commandPanel().subscribe(p_name, std::move(p_callback));
	}

	void MessageBox::setMinimalWidth(uint32_t p_width)
	{
		_minimalWidth = p_width;

		const spk::Vector2UInt commandPanelMinimalSize = _content.commandPanel().minimalSize();
		const spk::Vector2UInt textAreaMinimalSize =
			_content.textArea().computeMinimalSize(std::max(commandPanelMinimalSize.x, p_width));

		setMinimumContentSize(spk::Vector2UInt(std::max({textAreaMinimalSize.x, commandPanelMinimalSize.x, p_width}), textAreaMinimalSize.y + _content.layout().elementPadding().y + commandPanelMinimalSize.y));
	}

	InformationMessageBox::InformationMessageBox(const std::string &p_name, spk::Widget *p_parent) :
		spk::MessageBox(p_name, p_parent)
	{
		setTitle("Information");
		_button = addButton(p_name + "::closeButton", "Close");
		_contract = _button->subscribeToClick([this]() {
			close();
		});
	}

	spk::PushButton *InformationMessageBox::button() const
	{
		return _button;
	}

	RequestMessageBox::RequestMessageBox(const std::string &p_name, spk::Widget *p_parent) :
		spk::MessageBox(p_name, p_parent)
	{
		setTitle("Request");

		_firstButton = addButton(p_name + "::firstButton", "FirstButton");
		_secondButton = addButton(p_name + "::secondButton", "SecondButton");
		configure("Yes", []() {
		},
				  "No",
				  []() {
				  });
	}

	void RequestMessageBox::configure(
		std::string_view p_firstCaption,
		const std::function<void()> &p_firstAction,
		std::string_view p_secondCaption,
		const std::function<void()> &p_secondAction)
	{
		_firstButton->setText(p_firstCaption);
		_firstContract = _firstButton->subscribeToClick(
			[this, action = p_firstAction]() {
				if (action != nullptr)
				{
					action();
				}
				close();
			});

		_secondButton->setText(p_secondCaption);
		_secondContract = _secondButton->subscribeToClick(
			[this, action = p_secondAction]() {
				if (action != nullptr)
				{
					action();
				}
				close();
			});

		_requestCloseContract = subscribeTo(
			spk::IInterfaceWindow::Event::Close,
			[action = p_secondAction]() {
				if (action != nullptr)
				{
					action();
				}
			});
	}

	spk::PushButton *RequestMessageBox::firstButton() const
	{
		return _firstButton;
	}

	spk::PushButton *RequestMessageBox::secondButton() const
	{
		return _secondButton;
	}
}
