#pragma once

#include <functional>
#include <memory>
#include <string>
#include <vector>

#include "structures/graphics/texture/spk_sprite_sheet.hpp"
#include "structures/widget/spk_panel.hpp"
#include "structures/widget/spk_push_button.hpp"
#include "structures/widget/spk_widget.hpp"

namespace spk
{
	class MenuBar : public spk::Widget
	{
	public:
		class Menu : public spk::Widget
		{
		public:
			class Item : public spk::PushButton
			{
			public:
				Item(const std::string& p_name, spk::Widget* p_parent);
			};

			class Break : public spk::Widget
			{
			private:
				std::shared_ptr<spk::SpriteSheet> _spriteSheet;
				unsigned int _height = 5;

			protected:
				[[nodiscard]] spk::RenderUnit _buildRenderUnit() const override;

			public:
				Break(const std::string& p_name, spk::Widget* p_parent);

				void setSpriteSheet(std::shared_ptr<spk::SpriteSheet> p_spriteSheet);
				void setHeight(unsigned int p_height);
				[[nodiscard]] unsigned int height() const;
			};

		private:
			struct Element
			{
				std::unique_ptr<spk::Widget> item;
				spk::PushButton::Contract itemContract;
			};

			spk::Panel _backgroundFrame;
			std::vector<Element> _elements;

			void _refreshOwningBar();

		protected:
			void _onGeometryChange() override;
			void _onMouseButtonPressedEvent(spk::MouseButtonPressedEvent& p_event) override;

		public:
			Menu(const std::string& p_name, spk::Widget* p_parent);

			[[nodiscard]] size_t nbItem() const;

			Item* addItem(std::string_view p_itemName, std::function<void()> p_callback);
			Break* addBreak();
		};

	private:
		struct MenuEntry
		{
			std::unique_ptr<spk::PushButton> menuButton;
			spk::PushButton::Contract menuButtonContract;
			std::unique_ptr<Menu> menu;
		};

		unsigned int _height = 25;
		spk::Panel _backgroundFrame;
		std::vector<std::unique_ptr<MenuEntry>> _menus;

	protected:
		void _onGeometryChange() override;

	public:
		explicit MenuBar(const std::string& p_name, spk::Widget* p_parent = nullptr);

		void setHeight(unsigned int p_height);
		[[nodiscard]] unsigned int height() const;
		[[nodiscard]] size_t nbMenu() const;

		Menu* addMenu(std::string_view p_menuName);
		[[nodiscard]] spk::PushButton* menuButton(size_t p_index);
	};
}
