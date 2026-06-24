#include "structures/widget/spk_checkable_icon_button.hpp"

#include <algorithm>
#include <utility>

namespace spk
{
	namespace
	{
		constexpr size_t UncheckedPlaceholderIconSpriteID = 0;
		constexpr size_t DefaultCheckedIconSpriteID = 8;

		[[nodiscard]] std::shared_ptr<spk::SpriteSheet> defaultIconset()
		{
			return spk::WidgetStyle::Collection::style(spk::WidgetStyle::Collection::Default).iconSpriteSheet();
		}
	}

	CheckableIconButton::CheckableIconButton(const std::string &p_name, spk::Widget *p_parent) :
		spk::Widget(p_name, p_parent),
		_uncheckedButton(
			p_name + "::uncheckedButton",
			defaultIconset(),
			UncheckedPlaceholderIconSpriteID,
			this),
		_checkedButton(
			p_name + "::checkedButton",
			defaultIconset(),
			DefaultCheckedIconSpriteID,
			this)
	{
		_uncheckedButton.removeIcon();
		_setup();
	}

	CheckableIconButton::CheckableIconButton(
		const std::string &p_name,
		size_t p_uncheckedIconSpriteID,
		size_t p_checkedIconSpriteID,
		spk::Widget *p_parent) :
		spk::Widget(p_name, p_parent),
		_uncheckedButton(
			p_name + "::uncheckedButton",
			defaultIconset(),
			p_uncheckedIconSpriteID,
			this),
		_checkedButton(
			p_name + "::checkedButton",
			defaultIconset(),
			p_checkedIconSpriteID,
			this)
	{
		_setup();
	}

	void CheckableIconButton::_setup()
	{
		_stateContract = _isChecked.subscribe([this](const bool &) {
			_refreshState();
		});

		_uncheckedClickContract = _uncheckedButton.subscribeToClick([this]() {
			toggle();
		});

		_checkedClickContract = _checkedButton.subscribeToClick([this]() {
			toggle();
		});

		configureMinimalSizeGenerator([this]() {
			const spk::Vector2UInt uncheckedSize = _uncheckedButton.minimalSize();
			const spk::Vector2UInt checkedSize = _checkedButton.minimalSize();

			return spk::Vector2UInt(
				std::max(uncheckedSize.x, checkedSize.x),
				std::max(uncheckedSize.y, checkedSize.y));
		});

		_refreshState();
		activate();
	}

	void CheckableIconButton::_refreshState()
	{
		if (_isChecked.value() == true)
		{
			_uncheckedButton.deactivate();
			_checkedButton.activate();
		}
		else
		{
			_checkedButton.deactivate();
			_uncheckedButton.activate();
		}
	}

	void CheckableIconButton::_onGeometryChange()
	{
		const spk::Rect2D childRect(0, 0, geometry().width(), geometry().height());
		_uncheckedButton.setGeometry(childRect);
		_checkedButton.setGeometry(childRect);
	}

	void CheckableIconButton::applyStyle(const spk::WidgetStyle &p_style)
	{
		applyStyle(p_style, p_style);
	}

	void CheckableIconButton::applyStyle(const spk::WidgetStyle &p_uncheckedStyle, const spk::WidgetStyle &p_checkedStyle)
	{
		const bool uncheckedHadIcon = _uncheckedButton.hasIcon();
		const bool checkedHadIcon = _checkedButton.hasIcon();

		_uncheckedButton.applyStyle(p_uncheckedStyle);
		_checkedButton.applyStyle(p_checkedStyle);

		if (uncheckedHadIcon == false)
		{
			_uncheckedButton.removeIcon();
		}

		if (checkedHadIcon == false)
		{
			_checkedButton.removeIcon();
		}
	}

	bool CheckableIconButton::isChecked() const
	{
		return _isChecked.value();
	}

	void CheckableIconButton::setChecked(bool p_checked)
	{
		_isChecked = p_checked;
	}

	void CheckableIconButton::toggle()
	{
		_isChecked = (_isChecked.value() == false);
	}

	void CheckableIconButton::setUncheckedIconSpriteID(size_t p_spriteID)
	{
		_uncheckedButton.setIconSpriteID(p_spriteID);
	}

	void CheckableIconButton::setCheckedIconSpriteID(size_t p_spriteID)
	{
		_checkedButton.setIconSpriteID(p_spriteID);
	}

	size_t CheckableIconButton::uncheckedIconSpriteID() const
	{
		return _uncheckedButton.iconSpriteID();
	}

	size_t CheckableIconButton::checkedIconSpriteID() const
	{
		return _checkedButton.iconSpriteID();
	}

	spk::IconButton &CheckableIconButton::uncheckedButton()
	{
		return _uncheckedButton;
	}

	const spk::IconButton &CheckableIconButton::uncheckedButton() const
	{
		return _uncheckedButton;
	}

	spk::IconButton &CheckableIconButton::checkedButton()
	{
		return _checkedButton;
	}

	const spk::IconButton &CheckableIconButton::checkedButton() const
	{
		return _checkedButton;
	}

	CheckableIconButton::StateContract CheckableIconButton::subscribeToState(StateCallback p_callback)
	{
		return _isChecked.subscribe(std::move(p_callback));
	}

	CheckableIconButton::StateContract CheckableIconButton::addStateCallback(bool p_state, std::function<void()> p_callback)
	{
		return _isChecked.subscribe(
			[p_state, callback = std::move(p_callback)](const bool &p_newState) {
				if (p_newState == p_state && callback != nullptr)
				{
					callback();
				}
			});
	}
}
