#include "structures/widget/spk_action_bar.hpp"

#include <algorithm>
#include <stdexcept>
#include <utility>

#include "spk_generated_resources.hpp"
#include "structures/graphics/rendering/command/spk_sprite_render_command.hpp"
#include "structures/widget/spk_widget_style.hpp"

namespace
{
	[[nodiscard]] std::shared_ptr<spk::SpriteSheet> defaultBreakSpriteSheet()
	{
		static std::shared_ptr<spk::SpriteSheet> result = std::make_shared<spk::SpriteSheet>(
			spk::SpriteSheet::fromRawData(
				SPARKLE_GET_RESOURCE("resources/textures/default_break.png"),
				spk::Vector2UInt{3, 1}));
		return result;
	}

	constexpr int ItemSpacing = 5;
}

namespace spk
{
	MenuBar::Menu::Item::Item(const std::string& p_name, spk::Widget* p_parent) :
		spk::PushButton(p_name, p_parent)
	{
		setFlat(true);
		setAlignment(spk::HorizontalAlignment::Left, spk::VerticalAlignment::Centered);
	}

	MenuBar::Menu::Break::Break(const std::string& p_name, spk::Widget* p_parent) :
		spk::Widget(p_name, p_parent),
		_spriteSheet(defaultBreakSpriteSheet())
	{
		sizeHint().configureMinimalGenerator([this]() {
			return spk::Vector2UInt(1, _height);
		});
		activate();
	}

	spk::RenderUnit MenuBar::Menu::Break::_buildRenderUnit() const
	{
		spk::RenderUnitBuilder builder;

		if (_spriteSheet == nullptr || geometry().empty() == true)
		{
			return builder.build();
		}

		const spk::Rect2D& rect = geometry();
		const unsigned int capSize = std::min(_height, rect.width() / 2);
		const unsigned int middleWidth = rect.width() - capSize * 2;

		builder.emplace<spk::SpriteRenderCommand>(
			*_spriteSheet,
			spk::Vector2UInt{0, 0},
			spk::Rect2D(rect.anchor, spk::Vector2UInt(capSize, rect.height())));

		if (middleWidth > 0)
		{
			builder.emplace<spk::SpriteRenderCommand>(
				*_spriteSheet,
				spk::Vector2UInt{1, 0},
				spk::Rect2D(
					{rect.anchor.x + static_cast<int>(capSize), rect.anchor.y},
					spk::Vector2UInt(middleWidth, rect.height())));
		}

		builder.emplace<spk::SpriteRenderCommand>(
			*_spriteSheet,
			spk::Vector2UInt{2, 0},
			spk::Rect2D(
				{rect.anchor.x + static_cast<int>(rect.width() - capSize), rect.anchor.y},
				spk::Vector2UInt(capSize, rect.height())));

		return builder.build();
	}

	void MenuBar::Menu::Break::setSpriteSheet(std::shared_ptr<spk::SpriteSheet> p_spriteSheet)
	{
		if (p_spriteSheet == nullptr)
		{
			throw std::invalid_argument("Menu break sprite sheet cannot be null");
		}

		if (p_spriteSheet->nbSprite() != spk::Vector2UInt{3, 1})
		{
			throw std::invalid_argument("Menu break expects a 3x1 sprite sheet");
		}

		_spriteSheet = std::move(p_spriteSheet);
		invalidateRenderUnit();
	}

	void MenuBar::Menu::Break::setHeight(unsigned int p_height)
	{
		_height = p_height;
		invalidateRenderUnit();
	}

	unsigned int MenuBar::Menu::Break::height() const
	{
		return _height;
	}

	MenuBar::Menu::Menu(const std::string& p_name, spk::Widget* p_parent) :
		spk::Widget(p_name, p_parent),
		_backgroundFrame(p_name + "::backgroundFrame", this)
	{
		sizeHint().configureMinimalGenerator([this]() {
			spk::Vector2UInt result = {0, 0};

			for (const auto& element : _elements)
			{
				if (result.y != 0)
				{
					result.y += ItemSpacing;
				}

				const spk::Vector2UInt elementSize = element.item->minimalSize();
				result.x = std::max(result.x, elementSize.x + static_cast<unsigned int>(ItemSpacing * 2));
				result.y += elementSize.y;
			}

			const spk::Vector2Int cornerSize = _backgroundFrame.cornerSize();
			return result + spk::Vector2UInt(cornerSize.x * 2, cornerSize.y * 2);
		});
	}

	void MenuBar::Menu::_onGeometryChange()
	{
		_backgroundFrame.setGeometry(geometry().atOrigin());

		const spk::Vector2Int cornerSize = _backgroundFrame.cornerSize();
		spk::Vector2Int anchor = {cornerSize.x + ItemSpacing, cornerSize.y};
		const int contentWidth =
			static_cast<int>(geometry().width()) - cornerSize.x * 2 - ItemSpacing * 2;

		for (const auto& element : _elements)
		{
			const spk::Vector2UInt elementSize = element.item->minimalSize();

			element.item->setGeometry(spk::Rect2D(
				anchor,
				spk::Vector2UInt(static_cast<unsigned int>(std::max(0, contentWidth)), elementSize.y)));

			anchor.y += static_cast<int>(elementSize.y) + ItemSpacing;
		}
	}

	void MenuBar::Menu::_onMouseButtonPressedEvent(spk::MouseButtonPressedEvent& p_event)
	{
		if (absoluteGeometry().contains(p_event.device().position) == false)
		{
			deactivate();
		}
	}

	size_t MenuBar::Menu::nbItem() const
	{
		return _elements.size();
	}

	MenuBar::Menu::Item* MenuBar::Menu::addItem(std::string_view p_itemName, std::function<void()> p_callback)
	{
		auto item = std::make_unique<Item>(name() + "::" + std::string(p_itemName), this);

		item->setText(p_itemName);

		spk::PushButton::Contract contract = item->subscribeToClick(
			[this, callback = std::move(p_callback)]()
			{
				if (callback != nullptr)
				{
					callback();
				}
				deactivate();
			});

		Item* result = item.get();
		_elements.push_back({std::move(item), std::move(contract)});

		_refreshOwningBar();

		return result;
	}

	MenuBar::Menu::Break* MenuBar::Menu::addBreak()
	{
		auto item = std::make_unique<Break>(name() + "::break", this);

		Break* result = item.get();
		_elements.push_back({std::move(item), spk::PushButton::Contract()});

		_refreshOwningBar();

		return result;
	}

	void MenuBar::Menu::_refreshOwningBar()
	{
		MenuBar* bar = dynamic_cast<MenuBar*>(parent());
		if (bar != nullptr)
		{
			bar->_onGeometryChange();
		}
		else
		{
			_onGeometryChange();
		}
	}

	MenuBar::MenuBar(const std::string& p_name, spk::Widget* p_parent) :
		spk::Widget(p_name, p_parent),
		_backgroundFrame(p_name + "::backgroundFrame", this)
	{
		activate();
	}

	void MenuBar::_onGeometryChange()
	{
		_backgroundFrame.setGeometry(spk::Rect2D(0, 0, geometry().width(), _height));

		const spk::Vector2Int cornerSize = _backgroundFrame.cornerSize();
		spk::Vector2Int anchor = cornerSize;

		for (auto& entry : _menus)
		{
			const unsigned int buttonHeight =
				(_height > static_cast<unsigned int>(cornerSize.y * 2))
					? _height - static_cast<unsigned int>(cornerSize.y * 2)
					: _height;

			const spk::Vector2UInt buttonMinimal = entry->menuButton->minimalSize();
			const spk::Vector2UInt buttonSize = {buttonMinimal.x + static_cast<unsigned int>(ItemSpacing * 2), buttonHeight};

			entry->menuButton->setGeometry(spk::Rect2D(anchor, buttonSize));

			entry->menu->setGeometry(spk::Rect2D(
				{anchor.x, static_cast<int>(_height)},
				entry->menu->minimalSize()));

			anchor.x += static_cast<int>(buttonSize.x) + ItemSpacing;
		}
	}

	void MenuBar::setHeight(unsigned int p_height)
	{
		_height = p_height;
		_onGeometryChange();
	}

	unsigned int MenuBar::height() const
	{
		return _height;
	}

	size_t MenuBar::nbMenu() const
	{
		return _menus.size();
	}

	MenuBar::Menu* MenuBar::addMenu(std::string_view p_menuName)
	{
		std::unique_ptr<MenuEntry> newMenuEntry = std::make_unique<MenuEntry>();

		newMenuEntry->menuButton = std::make_unique<spk::PushButton>(
			name() + "::" + std::string(p_menuName) + "Button",
			p_menuName,
			this);
		newMenuEntry->menuButton->setFlat(true);

		newMenuEntry->menu = std::make_unique<Menu>(
			name() + "::" + std::string(p_menuName) + "Menu",
			this);

		MenuEntry* rawMenuEntryPtr = newMenuEntry.get();

		newMenuEntry->menuButtonContract = newMenuEntry->menuButton->subscribeToClick(
			[this, rawMenuEntryPtr]()
			{
				const bool wasActivated = rawMenuEntryPtr->menu->isActivated();

				for (auto& entry : _menus)
				{
					entry->menu->deactivate();
				}

				if (wasActivated == false && rawMenuEntryPtr->menu->nbItem() != 0)
				{
					rawMenuEntryPtr->menu->activate();
				}

				_onGeometryChange();
			});

		_menus.push_back(std::move(newMenuEntry));

		_onGeometryChange();

		return _menus.back()->menu.get();
	}

	spk::PushButton* MenuBar::menuButton(size_t p_index)
	{
		if (p_index >= _menus.size())
		{
			throw std::out_of_range("MenuBar menu button index out of range");
		}
		return _menus[p_index]->menuButton.get();
	}
}
