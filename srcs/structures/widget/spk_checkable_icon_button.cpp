#include "structures/widget/spk_checkable_icon_button.hpp"

#include <utility>

namespace spk
{
	CheckableIconButton::CheckableIconButton(const std::string& p_name, spk::Widget* p_parent) :
		CheckableIconButton(p_name, 8, 9, p_parent)
	{
	}

	CheckableIconButton::CheckableIconButton(
		const std::string& p_name,
		size_t p_uncheckedIconSpriteID,
		size_t p_checkedIconSpriteID,
		spk::Widget* p_parent) :
		spk::IconButton(p_name, p_parent),
		_uncheckedIconSpriteID(p_uncheckedIconSpriteID),
		_checkedIconSpriteID(p_checkedIconSpriteID)
	{
		_stateContract = _isChecked.subscribe([this](const bool&)
		{
			_applyStateIcon();
		});

		_toggleContract = subscribeToClick([this]()
		{
			toggle();
		});

		_applyStateIcon();
	}

	void CheckableIconButton::_applyStateIcon()
	{
		setIconSpriteID((_isChecked.value() == true) ? _checkedIconSpriteID : _uncheckedIconSpriteID);
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
		_uncheckedIconSpriteID = p_spriteID;
		_applyStateIcon();
	}

	void CheckableIconButton::setCheckedIconSpriteID(size_t p_spriteID)
	{
		_checkedIconSpriteID = p_spriteID;
		_applyStateIcon();
	}

	size_t CheckableIconButton::uncheckedIconSpriteID() const
	{
		return _uncheckedIconSpriteID;
	}

	size_t CheckableIconButton::checkedIconSpriteID() const
	{
		return _checkedIconSpriteID;
	}

	CheckableIconButton::StateContract CheckableIconButton::subscribeToState(StateCallback p_callback)
	{
		return _isChecked.subscribe(std::move(p_callback));
	}

	CheckableIconButton::StateContract CheckableIconButton::addStateCallback(bool p_state, std::function<void()> p_callback)
	{
		return _isChecked.subscribe(
			[p_state, callback = std::move(p_callback)](const bool& p_newState)
			{
				if (p_newState == p_state && callback != nullptr)
				{
					callback();
				}
			});
	}
}
