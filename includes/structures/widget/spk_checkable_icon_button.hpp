#pragma once

#include <string>

#include "structures/container/spk_observable_value.hpp"
#include "structures/widget/spk_icon_button.hpp"

namespace spk
{
	class CheckableIconButton : public spk::IconButton
	{
	public:
		using StateCallback = spk::ObservableValue<bool>::Callback;
		using StateContract = spk::ObservableValue<bool>::Contract;

	private:
		spk::ObservableValue<bool> _isChecked{false};
		size_t _uncheckedIconSpriteID = 0;
		size_t _checkedIconSpriteID = 0;
		StateContract _stateContract;
		spk::PushButton::Contract _toggleContract;

		using spk::IconButton::setIconSpriteID;

		void _applyStateIcon();

	public:
		explicit CheckableIconButton(const std::string& p_name, spk::Widget* p_parent = nullptr);
		CheckableIconButton(
			const std::string& p_name,
			size_t p_uncheckedIconSpriteID,
			size_t p_checkedIconSpriteID,
			spk::Widget* p_parent = nullptr);

		[[nodiscard]] bool isChecked() const;
		void setChecked(bool p_checked);
		void toggle();

		void setUncheckedIconSpriteID(size_t p_spriteID);
		void setCheckedIconSpriteID(size_t p_spriteID);
		[[nodiscard]] size_t uncheckedIconSpriteID() const;
		[[nodiscard]] size_t checkedIconSpriteID() const;

		StateContract subscribeToState(StateCallback p_callback);
		StateContract addStateCallback(bool p_state, std::function<void()> p_callback);
	};
}
