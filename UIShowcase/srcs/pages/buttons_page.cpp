#include "pages/buttons_page.hpp"

#include <array>
#include <string>
#include <utility>

namespace showcase
{
	namespace
	{
		// Iconset sprite ids in the default 10x10 iconset (see WidgetStyle default style):
		// 4 = up, 5 = down, 6 = left, 7 = right.
		constexpr std::array<std::pair<std::string_view, std::size_t>, 4> iconButtons = {{
			{"left", 6},
			{"right", 7},
			{"up", 4},
			{"down", 5},
		}};
	}

	std::string_view ButtonsPage::title() const
	{
		return "Buttons / Commands";
	}

	std::string_view ButtonsPage::description() const
	{
		return "Clickable controls: PushButton, IconButton, CheckableIconButton and CommandPanel.";
	}

	void ButtonsPage::buildContent(spk::Widget &p_parent)
	{
		_contentRoot = std::make_unique<VerticalStack>("Buttons::content", &p_parent);
		_contentBag.clear();
		_clickContracts.clear();
		_clickCount = 0;

		appendLabel(*_contentRoot, _contentBag, "Buttons::intro", description(), theme::bodyStyle());

		// PushButton with live click feedback.
		appendHeading(*_contentRoot, _contentBag, "Buttons::pushHeading", "PushButton");
		HorizontalStack &pushRow = appendRow(*_contentRoot, _contentBag, "Buttons::pushRow", 34);
		{
			auto button = std::make_unique<spk::PushButton>(
				"Buttons::push",
				"Click me",
				theme::buttonReleasedStyle(),
				theme::buttonPressedStyle(),
				&pushRow);
			_clickContracts.push_back(button->subscribeToClick([this]() {
				++_clickCount;
				if (_clickStatus != nullptr)
				{
					_clickStatus->setText("Clicked " + std::to_string(_clickCount) + " time(s)");
				}
			}));
			adopt(pushRow.layout(), _contentBag, std::move(button), spk::Layout::SizePolicy::Fixed, spk::Vector2UInt(120, 0));

			auto status = std::make_unique<spk::TextLabel>("Buttons::clickStatus", "Not clicked yet", theme::accentStyle(), &pushRow);
			status->setAlignment(spk::HorizontalAlignment::Left, spk::VerticalAlignment::Centered);
			_clickStatus = status.get();
			adopt(pushRow.layout(), _contentBag, std::move(status), spk::Layout::SizePolicy::Extend);
		}

		// IconButtons drawn from the default iconset.
		appendHeading(*_contentRoot, _contentBag, "Buttons::iconHeading", "IconButton");
		HorizontalStack &iconRow = appendRow(*_contentRoot, _contentBag, "Buttons::iconRow", 36);
		for (const auto &[name, spriteID] : iconButtons)
		{
			auto button = std::make_unique<spk::IconButton>("Buttons::icon::" + std::string(name), &iconRow);
			button->setIconSpriteID(spriteID);
			adopt(iconRow.layout(), _contentBag, std::move(button), spk::Layout::SizePolicy::Fixed, spk::Vector2UInt(36, 0));
		}

		// CheckableIconButton with state feedback.
		appendHeading(*_contentRoot, _contentBag, "Buttons::checkHeading", "CheckableIconButton");
		HorizontalStack &checkRow = appendRow(*_contentRoot, _contentBag, "Buttons::checkRow", 36);
		{
			auto check = std::make_unique<spk::CheckableIconButton>("Buttons::check", 5, 4, &checkRow);
			_checkContract = check->subscribeToState([this](const bool &p_checked) {
				if (_checkStatus != nullptr)
				{
					_checkStatus->setText(p_checked == true ? "State: checked" : "State: unchecked");
				}
			});
			adopt(checkRow.layout(), _contentBag, std::move(check), spk::Layout::SizePolicy::Fixed, spk::Vector2UInt(36, 0));

			auto status = std::make_unique<spk::TextLabel>("Buttons::checkStatus", "State: unchecked", theme::accentStyle(), &checkRow);
			status->setAlignment(spk::HorizontalAlignment::Left, spk::VerticalAlignment::Centered);
			_checkStatus = status.get();
			adopt(checkRow.layout(), _contentBag, std::move(status), spk::Layout::SizePolicy::Extend);
		}

		// CommandPanel: a managed, right-aligned group of buttons.
		appendHeading(*_contentRoot, _contentBag, "Buttons::commandHeading", "CommandPanel");
		{
			auto command = std::make_unique<spk::CommandPanel>("Buttons::command", _contentRoot.get());
			command->activate();
			command->addButton("ok", "OK");
			command->addButton("cancel", "Cancel");
			_clickContracts.push_back(command->subscribe("ok", [this]() {
				if (_clickStatus != nullptr)
				{
					_clickStatus->setText("CommandPanel: OK");
				}
			}));
			_clickContracts.push_back(command->subscribe("cancel", [this]() {
				if (_clickStatus != nullptr)
				{
					_clickStatus->setText("CommandPanel: Cancel");
				}
			}));
			adopt(_contentRoot->layout(), _contentBag, std::move(command), spk::Layout::SizePolicy::Fixed, spk::Vector2UInt(0, 34));
		}

		appendSpacer(*_contentRoot, _contentBag, "Buttons::spacer");
	}

	void ButtonsPage::buildProperties(spk::Widget &p_parent)
	{
		buildStatusProperties(
			p_parent,
			_propertiesRoot,
			_propertiesBag,
			"Buttons",
			"implemented",
			{
				{"PushButton", "click contract, pressed style"},
				{"IconButton", "default iconset sprite"},
				{"CheckableIconButton", "observable boolean"},
				{"CommandPanel", "named button group"},
			});
	}
}
