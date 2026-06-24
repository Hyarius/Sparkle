#include "structures/widget/spk_prompt_panel.hpp"

#include <algorithm>
#include <utility>

namespace spk
{
	PromptPanel::PromptPanel(const std::string &p_name, spk::Widget *p_parent) :
		spk::Widget(p_name, p_parent),
		_background(p_name + "::background", this),
		_textArea(p_name + "::textArea", this),
		_commandPanel(p_name + "::commandPanel", this)
	{
		_layout.setElementPadding({8, 8});
		_layout.addWidget(&_textArea, spk::Layout::SizePolicy::Extend);
		_layout.addWidget(&_commandPanel, spk::Layout::SizePolicy::Minimum);

		configureMinimalSizeGenerator([this]() {
			const spk::Vector2Int cornerSize = _background.cornerSize();
			const spk::Vector2UInt commandPanelSize = _commandPanel.minimalSize();

			spk::Vector2UInt contentSize = commandPanelSize;
			if (_textArea.text().empty() == false)
			{
				const spk::Vector2UInt textAreaSize = _textArea.computeMinimalSize(
					std::max(commandPanelSize.x, 200u));

				contentSize.x = std::max(textAreaSize.x, commandPanelSize.x);
				contentSize.y = textAreaSize.y + _layout.elementPadding().y + commandPanelSize.y;
			}

			return contentSize + spk::Vector2UInt(cornerSize.x * 2, cornerSize.y * 2);
		});

		activate();
	}

	void PromptPanel::_onGeometryChange()
	{
		_background.setGeometry(geometry().atOrigin());

		const spk::Vector2Int cornerSize = _background.cornerSize();
		_layout.setGeometry(geometry().atOrigin().shrink(cornerSize));
	}

	void PromptPanel::setMessage(std::string_view p_text)
	{
		_textArea.setText(p_text);
	}

	const spk::Font::Text &PromptPanel::message() const
	{
		return _textArea.text();
	}

	spk::PushButton *PromptPanel::addButton(const std::string &p_name, std::string_view p_label)
	{
		spk::PushButton *result = _commandPanel.addButton(p_name, p_label);
		_onGeometryChange();
		return result;
	}

	spk::PushButton *PromptPanel::button(const std::string &p_name)
	{
		return _commandPanel.button(p_name);
	}

	void PromptPanel::removeButton(const std::string &p_name)
	{
		_commandPanel.removeButton(p_name);
		_onGeometryChange();
	}

	spk::CommandPanel::Contract PromptPanel::subscribe(const std::string &p_name, spk::CommandPanel::Callback p_callback)
	{
		return _commandPanel.subscribe(p_name, std::move(p_callback));
	}

	void PromptPanel::setButtonPadding(const spk::Vector2UInt &p_padding)
	{
		_commandPanel.setElementPadding(p_padding);
	}

	void PromptPanel::setButtonSizePolicy(spk::Layout::SizePolicy p_policy)
	{
		_commandPanel.setSizePolicy(p_policy);
	}

	spk::Layout::SizePolicy PromptPanel::buttonSizePolicy() const
	{
		return _commandPanel.sizePolicy();
	}

	spk::Panel &PromptPanel::background()
	{
		return _background;
	}

	const spk::Panel &PromptPanel::background() const
	{
		return _background;
	}

	spk::TextArea &PromptPanel::textArea()
	{
		return _textArea;
	}

	const spk::TextArea &PromptPanel::textArea() const
	{
		return _textArea;
	}

	spk::CommandPanel &PromptPanel::commandPanel()
	{
		return _commandPanel;
	}

	const spk::CommandPanel &PromptPanel::commandPanel() const
	{
		return _commandPanel;
	}
}
