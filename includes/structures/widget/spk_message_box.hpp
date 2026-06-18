#pragma once

#include <functional>
#include <string>

#include "structures/widget/spk_command_panel.hpp"
#include "structures/widget/spk_interface_window.hpp"
#include "structures/widget/spk_linear_layout.hpp"
#include "structures/widget/spk_text_area.hpp"

#ifdef MessageBox
#	undef MessageBox
#endif

namespace spk
{
	class MessageBox : public spk::IInterfaceWindow
	{
	public:
		class Content : public spk::Widget
		{
		private:
			spk::VerticalLayout _layout;
			spk::TextArea _textArea;
			spk::CommandPanel _commandPanel;

			void _compose();

		protected:
			void _onGeometryChange() override;

		public:
			Content(const std::string& p_name, spk::Widget* p_parent);

			[[nodiscard]] spk::VerticalLayout& layout();
			[[nodiscard]] const spk::VerticalLayout& layout() const;
			[[nodiscard]] spk::TextArea& textArea();
			[[nodiscard]] const spk::TextArea& textArea() const;
			[[nodiscard]] spk::CommandPanel& commandPanel();
			[[nodiscard]] const spk::CommandPanel& commandPanel() const;
		};

	private:
		Content _content;
		uint32_t _minimalWidth = 0;

		spk::IInterfaceWindow::ResizeContract _onResizeContract;
		spk::IInterfaceWindow::EventContract _closeContract;

	public:
		explicit MessageBox(const std::string& p_name, spk::Widget* p_parent = nullptr);

		[[nodiscard]] Content& content();
		[[nodiscard]] const Content& content() const;
		[[nodiscard]] spk::TextArea& textArea();
		[[nodiscard]] const spk::TextArea& textArea() const;
		[[nodiscard]] spk::CommandPanel& commandPanel();
		[[nodiscard]] const spk::CommandPanel& commandPanel() const;

		virtual void setText(std::string_view p_text);
		[[nodiscard]] const spk::Font::Text& text() const;

		spk::PushButton* addButton(const std::string& p_name, std::string_view p_label);
		[[nodiscard]] spk::PushButton* button(const std::string& p_name);
		[[nodiscard]] const spk::PushButton* button(const std::string& p_name) const;
		void removeButton(const std::string& p_name);

		spk::CommandPanel::Contract subscribe(const std::string& p_name, spk::CommandPanel::Callback p_callback);

		void setMinimalWidth(uint32_t p_width);
	};

	class InformationMessageBox : public spk::MessageBox
	{
	private:
		spk::PushButton* _button = nullptr;
		spk::PushButton::Contract _contract;

	public:
		explicit InformationMessageBox(const std::string& p_name, spk::Widget* p_parent = nullptr);

		[[nodiscard]] spk::PushButton* button() const;
	};

	class RequestMessageBox : public spk::MessageBox
	{
	private:
		spk::PushButton* _firstButton = nullptr;
		spk::PushButton::Contract _firstContract;

		spk::PushButton* _secondButton = nullptr;
		spk::PushButton::Contract _secondContract;

		spk::IInterfaceWindow::EventContract _requestCloseContract;

	public:
		explicit RequestMessageBox(const std::string& p_name, spk::Widget* p_parent = nullptr);

		void configure(
			std::string_view p_firstCaption,
			const std::function<void()>& p_firstAction,
			std::string_view p_secondCaption,
			const std::function<void()>& p_secondAction);

		[[nodiscard]] spk::PushButton* firstButton() const;
		[[nodiscard]] spk::PushButton* secondButton() const;
	};
}
