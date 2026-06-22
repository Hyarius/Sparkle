#pragma once

#include <functional>
#include <string>

#include "structures/container/spk_observable_value.hpp"
#include "structures/widget/spk_icon_button.hpp"
#include "structures/widget/spk_widget.hpp"
#include "structures/widget/spk_widget_style.hpp"

namespace spk
{
	class CheckableIconButton : public spk::Widget
	{
	public:
		using StateCallback = spk::ObservableValue<bool>::Callback;
		using StateContract = spk::ObservableValue<bool>::Contract;

	private:
		spk::ObservableValue<bool> _isChecked{false};

		spk::IconButton _uncheckedButton;
		spk::IconButton _checkedButton;

		StateContract _stateContract;
		spk::PushButton::Contract _uncheckedClickContract;
		spk::PushButton::Contract _checkedClickContract;

		void _refreshState();

	protected:
		void _onGeometryChange() override;

	public:
		explicit CheckableIconButton(const std::string &p_name, spk::Widget *p_parent = nullptr);
		CheckableIconButton(
			const std::string &p_name,
			size_t p_uncheckedIconSpriteID,
			size_t p_checkedIconSpriteID,
			spk::Widget *p_parent = nullptr);

		void applyStyle(const spk::WidgetStyle &p_style) override;
		void applyStyle(const spk::WidgetStyle &p_uncheckedStyle, const spk::WidgetStyle &p_checkedStyle);

		[[nodiscard]] bool isChecked() const;
		void setChecked(bool p_checked);
		void toggle();

		void setUncheckedIconSpriteID(size_t p_spriteID);
		void setCheckedIconSpriteID(size_t p_spriteID);
		[[nodiscard]] size_t uncheckedIconSpriteID() const;
		[[nodiscard]] size_t checkedIconSpriteID() const;

		[[nodiscard]] spk::IconButton &uncheckedButton();
		[[nodiscard]] const spk::IconButton &uncheckedButton() const;
		[[nodiscard]] spk::IconButton &checkedButton();
		[[nodiscard]] const spk::IconButton &checkedButton() const;

		StateContract subscribeToState(StateCallback p_callback);
		StateContract addStateCallback(bool p_state, std::function<void()> p_callback);
	};
}
