#include "pages/input_page.hpp"

#include <string>

namespace showcase
{
	namespace
	{
		// Append a "caption  [field]" row: a fixed-width caption on the left and a field that
		// fills the remaining width. Returns the row so the caller can place the field into it.
		HorizontalStack &appendFieldRow(
			VerticalStack &p_stack,
			WidgetBag &p_bag,
			const std::string &p_name,
			std::string_view p_caption)
		{
			HorizontalStack &row = appendRow(p_stack, p_bag, p_name, 30);

			auto caption = std::make_unique<spk::TextLabel>(p_name + "::caption", p_caption, theme::bodyStyle(), &row);
			caption->setAlignment(spk::HorizontalAlignment::Left, spk::VerticalAlignment::Centered);
			adopt(row.layout(), p_bag, std::move(caption), spk::Layout::SizePolicy::Fixed, spk::Vector2UInt(120, 0));

			return row;
		}
	}

	std::string_view InputPage::title() const
	{
		return "Input";
	}

	std::string_view InputPage::description() const
	{
		return "Editable controls: spk::TextEdit and spk::NumericSpinBox.";
	}

	void InputPage::buildContent(spk::Widget &p_parent)
	{
		_contentRoot = std::make_unique<VerticalStack>("Input::content", &p_parent);
		_contentBag.clear();

		appendLabel(*_contentRoot, _contentBag, "Input::intro", description(), theme::bodyStyle());

		appendHeading(*_contentRoot, _contentBag, "Input::textHeading", "TextEdit");

		// Plain text edit with a placeholder and a live echo of its content.
		HorizontalStack &textRow = appendFieldRow(*_contentRoot, _contentBag, "Input::textRow", "Text");
		{
			auto edit = std::make_unique<spk::TextEdit>("Input::text", &textRow);
			edit->setPlaceholder("Type something...");
			spk::TextEdit &editReference = adopt(textRow.layout(), _contentBag, std::move(edit), spk::Layout::SizePolicy::Extend);
			// Capture by reference so the callback can read the rendered UTF-8 text back.
			_textContract = editReference.subscribeToEdition([this, &editReference](const spk::Font::Text &) {
				if (_echoLabel != nullptr)
				{
					const std::string text = editReference.textAsUTF8();
					_echoLabel->setText(text.empty() == true ? "(empty)" : text);
				}
			});
		}

		HorizontalStack &echoRow = appendFieldRow(*_contentRoot, _contentBag, "Input::echoRow", "Echo");
		{
			auto echo = std::make_unique<spk::TextLabel>("Input::echo", "(empty)", theme::accentStyle(), &echoRow);
			echo->setAlignment(spk::HorizontalAlignment::Left, spk::VerticalAlignment::Centered);
			_echoLabel = echo.get();
			adopt(echoRow.layout(), _contentBag, std::move(echo), spk::Layout::SizePolicy::Extend);
		}

		// Obscured text edit for passwords.
		HorizontalStack &passwordRow = appendFieldRow(*_contentRoot, _contentBag, "Input::passwordRow", "Password");
		{
			auto edit = std::make_unique<spk::TextEdit>("Input::password", &passwordRow);
			edit->setPlaceholder("Obscured input");
			edit->setObscured(true);
			adopt(passwordRow.layout(), _contentBag, std::move(edit), spk::Layout::SizePolicy::Extend);
		}

		appendHeading(*_contentRoot, _contentBag, "Input::spinHeading", "NumericSpinBox");

		// Integer spin box.
		HorizontalStack &intRow = appendFieldRow(*_contentRoot, _contentBag, "Input::intRow", "Integer");
		{
			auto spin = std::make_unique<spk::IntSpinBox>("Input::int", &intRow);
			spin->setValue(10);
			spin->setStep(1);
			_intContract = spin->subscribeToEdition([this](const int &p_value) {
				if (_intLabel != nullptr)
				{
					_intLabel->setText("= " + std::to_string(p_value));
				}
			});
			adopt(intRow.layout(), _contentBag, std::move(spin), spk::Layout::SizePolicy::Fixed, spk::Vector2UInt(140, 0));

			auto label = std::make_unique<spk::TextLabel>("Input::intValue", "= 10", theme::accentStyle(), &intRow);
			label->setAlignment(spk::HorizontalAlignment::Left, spk::VerticalAlignment::Centered);
			_intLabel = label.get();
			adopt(intRow.layout(), _contentBag, std::move(label), spk::Layout::SizePolicy::Extend);
		}

		// Floating-point spin box.
		HorizontalStack &floatRow = appendFieldRow(*_contentRoot, _contentBag, "Input::floatRow", "Float");
		{
			auto spin = std::make_unique<spk::FloatSpinBox>("Input::float", &floatRow);
			spin->setValue(2.5f);
			spin->setStep(0.5f);
			_floatContract = spin->subscribeToEdition([this](const float &p_value) {
				if (_floatLabel != nullptr)
				{
					_floatLabel->setText("= " + std::to_string(p_value));
				}
			});
			adopt(floatRow.layout(), _contentBag, std::move(spin), spk::Layout::SizePolicy::Fixed, spk::Vector2UInt(140, 0));

			auto label = std::make_unique<spk::TextLabel>("Input::floatValue", "= 2.5", theme::accentStyle(), &floatRow);
			label->setAlignment(spk::HorizontalAlignment::Left, spk::VerticalAlignment::Centered);
			_floatLabel = label.get();
			adopt(floatRow.layout(), _contentBag, std::move(label), spk::Layout::SizePolicy::Extend);
		}

		appendSpacer(*_contentRoot, _contentBag, "Input::spacer");
	}

	void InputPage::buildProperties(spk::Widget &p_parent)
	{
		buildStatusProperties(
			p_parent,
			_propertiesRoot,
			_propertiesBag,
			"Input",
			"implemented",
			{
				{"TextEdit", "placeholder, obscured, edition"},
				{"IntSpinBox", "step + observable value"},
				{"FloatSpinBox", "step + observable value"},
			});
	}
}
