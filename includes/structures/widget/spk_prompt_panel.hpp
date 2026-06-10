#pragma once

#include <string>
#include <string_view>

#include "structures/widget/spk_command_panel.hpp"
#include "structures/widget/spk_linear_layout.hpp"
#include "structures/widget/spk_panel.hpp"
#include "structures/widget/spk_text_area.hpp"
#include "structures/widget/spk_widget.hpp"

namespace spk
{
	class PromptPanel : public spk::Widget
	{
	private:
		spk::Panel _background;
		spk::TextArea _textArea;
		spk::CommandPanel _commandPanel;
		spk::VerticalLayout _layout;

	protected:
		void _onGeometryChange() override;

	public:
		explicit PromptPanel(const std::string& p_name, spk::Widget* p_parent = nullptr);

		void setMessage(std::string_view p_text);
		[[nodiscard]] const spk::Font::Text& message() const;

		spk::PushButton* addButton(const std::string& p_name, std::string_view p_label);
		[[nodiscard]] spk::PushButton* button(const std::string& p_name);
		void removeButton(const std::string& p_name);

		spk::CommandPanel::Contract subscribe(const std::string& p_name, spk::CommandPanel::Callback p_callback);

		void setButtonPadding(const spk::Vector2UInt& p_padding);
		void setButtonSizePolicy(spk::Layout::SizePolicy p_policy);
		[[nodiscard]] spk::Layout::SizePolicy buttonSizePolicy() const;

		[[nodiscard]] spk::Panel& background();
		[[nodiscard]] const spk::Panel& background() const;
		[[nodiscard]] spk::TextArea& textArea();
		[[nodiscard]] const spk::TextArea& textArea() const;
		[[nodiscard]] spk::CommandPanel& commandPanel();
		[[nodiscard]] const spk::CommandPanel& commandPanel() const;
	};
}
