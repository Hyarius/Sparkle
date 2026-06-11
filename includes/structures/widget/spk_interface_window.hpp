#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <type_traits>

#include "structures/design_pattern/spk_contract_provider.hpp"
#include "structures/widget/spk_panel.hpp"
#include "structures/widget/spk_push_button.hpp"
#include "structures/widget/spk_scalable_widget.hpp"
#include "structures/widget/spk_text_label.hpp"

namespace spk
{
	class IInterfaceWindow : public spk::ScalableWidget
	{
	public:
		using ResizeContractProvider = spk::ContractProvider<const spk::Vector2UInt&>;
		using ResizeContract = ResizeContractProvider::Contract;
		using ResizeCallback = ResizeContractProvider::Callback;
		using EventContract = spk::ContractProvider<>::Contract;
		using EventCallback = spk::ContractProvider<>::Callback;

		struct ContentPadding
		{
			std::uint32_t left = 0;
			std::uint32_t top = 0;
			std::uint32_t right = 0;
			std::uint32_t bottom = 0;

			[[nodiscard]] bool operator==(const ContentPadding&) const noexcept = default;
		};

		enum class Event
		{
			Minimize,
			Maximize,
			Close
		};

		class MenuBar : public spk::Widget
		{
			friend class IInterfaceWindow;

		public:
			enum class Button
			{
				Minimize,
				Maximize,
				Close
			};

			static constexpr int Margin = 3;

		private:
			spk::TextLabel _titleLabel;
			spk::PushButton _minimizeButton;
			spk::PushButton _maximizeButton;
			spk::PushButton _closeButton;

			[[nodiscard]] unsigned int _minimumControlButtonSize() const;
			[[nodiscard]] unsigned int _controlButtonSize() const;

			void _onGeometryChange() override;

			void _activateMenuButton(Button p_button);
			void _deactivateMenuButton(Button p_button);

		public:
			MenuBar(const std::string& p_name, spk::Widget* p_parent);

			[[nodiscard]] spk::TextLabel& titleLabel();
			[[nodiscard]] const spk::TextLabel& titleLabel() const;
			[[nodiscard]] spk::PushButton& minimizeButton();
			[[nodiscard]] const spk::PushButton& minimizeButton() const;
			[[nodiscard]] spk::PushButton& maximizeButton();
			[[nodiscard]] const spk::PushButton& maximizeButton() const;
			[[nodiscard]] spk::PushButton& closeButton();
			[[nodiscard]] const spk::PushButton& closeButton() const;
		};

	private:
		spk::Panel _backgroundFrame;
		spk::Panel _minimizedBackgroundFrame;
		MenuBar _menuBar;
		spk::Widget* _content = nullptr;
		std::optional<ContentPadding> _contentPadding;

		unsigned int _menuHeight = 20;

		bool _isMoving = false;
		spk::Vector2Int _moveOrigin;

		ResizeContractProvider _onResizeProvider;

		spk::PushButton::Contract _minimizeContract;
		spk::PushButton::Contract _maximizeContract;

		bool _isMinimized = false;
		bool _isMaximized = false;
		spk::Rect2D _previousGeometry;

	protected:
		[[nodiscard]] unsigned int _effectiveMenuHeight() const;
		[[nodiscard]] ContentPadding _effectiveContentPadding() const;

		void _onGeometryChange() override;
		void _onMouseMovedEvent(spk::MouseMovedEvent& p_event) override;
		void _onMouseButtonPressedEvent(spk::MouseButtonPressedEvent& p_event) override;
		void _onMouseButtonReleasedEvent(spk::MouseButtonReleasedEvent& p_event) override;

	public:
		explicit IInterfaceWindow(const std::string& p_name, spk::Widget* p_parent = nullptr);

		ResizeContract subscribeOnResize(ResizeCallback p_callback);
		EventContract subscribeTo(Event p_event, EventCallback p_callback);

		[[nodiscard]] MenuBar& menuBar();
		[[nodiscard]] const MenuBar& menuBar() const;
		[[nodiscard]] spk::Panel& backgroundFrame();
		[[nodiscard]] const spk::Panel& backgroundFrame() const;
		[[nodiscard]] spk::Panel& minimizedBackgroundFrame();
		[[nodiscard]] const spk::Panel& minimizedBackgroundFrame() const;

		void setContent(spk::Widget* p_content);
		[[nodiscard]] spk::Widget* content();
		[[nodiscard]] const spk::Widget* content() const;

		void setTitle(std::string_view p_title);
		void setContentPadding(const ContentPadding& p_padding);
		void resetContentPadding();
		void setMinimumContentSize(const spk::Vector2UInt& p_minimumContentSize);
		void setMenuHeight(unsigned int p_menuHeight);
		[[nodiscard]] ContentPadding contentPadding() const;
		[[nodiscard]] unsigned int menuHeight() const;
		[[nodiscard]] spk::Vector2UInt contentSize() const;

		void minimize();
		void maximize();
		void close();

		[[nodiscard]] bool isMinimized() const;
		[[nodiscard]] bool isMaximized() const;
		[[nodiscard]] bool isMoving() const;

		void activateMenuButton(MenuBar::Button p_button);
		void deactivateMenuButton(MenuBar::Button p_button);
	};

	template <typename TContentType>
		requires std::is_base_of_v<spk::Widget, TContentType>
	class InterfaceWindow : public spk::IInterfaceWindow
	{
	private:
		TContentType _contentObject;

		using spk::IInterfaceWindow::setContent;

	public:
		explicit InterfaceWindow(const std::string& p_name, spk::Widget* p_parent = nullptr) :
			spk::IInterfaceWindow(p_name, p_parent),
			_contentObject(p_name + "::content", &backgroundFrame())
		{
			setContent(&_contentObject);
			_contentObject.activate();
		}

		[[nodiscard]] TContentType& contentObject()
		{
			return _contentObject;
		}

		[[nodiscard]] const TContentType& contentObject() const
		{
			return _contentObject;
		}
	};
}
