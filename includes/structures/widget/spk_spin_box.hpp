#pragma once

#include <algorithm>
#include <optional>
#include <string>
#include <type_traits>

#include "structures/container/spk_observable_value.hpp"
#include "structures/widget/spk_push_button.hpp"
#include "structures/widget/spk_text_edit.hpp"
#include "structures/widget/spk_widget.hpp"

namespace spk
{
	template <typename TType>
		requires std::is_arithmetic_v<TType>
	class SpinBox : public spk::Widget
	{
	public:
		using Callback = spk::ContractProvider<const TType&>::Callback;
		using Contract = spk::ContractProvider<const TType&>::Contract;

	private:
		spk::PushButton _downButton;
		spk::PushButton::Contract _downButtonContract;
		spk::TextEdit _valueEdit;
		spk::PushButton _upButton;
		spk::PushButton::Contract _upButtonContract;

		std::optional<TType> _minLimit;
		std::optional<TType> _maxLimit;
		spk::ObservableValue<TType> _value;
		Contract _onValueEditionContract;
		TType _step = static_cast<TType>(1);

		void _refreshValueText()
		{
			_valueEdit.setText(std::to_string(_value.value()));
		}

		[[nodiscard]] TType _clampedValue(TType p_value) const
		{
			if (_minLimit.has_value() == true)
			{
				p_value = std::max(p_value, _minLimit.value());
			}
			if (_maxLimit.has_value() == true)
			{
				p_value = std::min(p_value, _maxLimit.value());
			}
			return p_value;
		}

	protected:
		void _onGeometryChange() override
		{
			const unsigned int buttonSize = geometry().height();
			const unsigned int editWidth = (geometry().width() > buttonSize * 2) ? geometry().width() - buttonSize * 2 : 0;

			_downButton.setGeometry(spk::Rect2D(0, 0, buttonSize, buttonSize));
			_valueEdit.setGeometry(spk::Rect2D(static_cast<int>(buttonSize), 0, editWidth, geometry().height()));
			_upButton.setGeometry(spk::Rect2D(static_cast<int>(buttonSize + editWidth), 0, buttonSize, buttonSize));
		}

	public:
		explicit SpinBox(const std::string& p_name, spk::Widget* p_parent = nullptr) :
			spk::Widget(p_name, p_parent),
			_downButton(p_name + "::downButton", "-", this),
			_valueEdit(p_name + "::valueEdit", this),
			_upButton(p_name + "::upButton", "+", this),
			_value(static_cast<TType>(0))
		{
			_onValueEditionContract = _value.subscribe([this](const TType&) { _refreshValueText(); });

			_downButtonContract = _downButton.subscribeToClick([this]() { _value = _clampedValue(_value.value() - _step); });

			_upButtonContract = _upButton.subscribeToClick([this]() { _value = _clampedValue(_value.value() + _step); });

			_valueEdit.setPlaceholder("...");
			_valueEdit.disableEdit();
			_refreshValueText();

			const auto& iconset = spk::WidgetStyle::Collection::style(spk::WidgetStyle::Collection::Default).iconSpriteSheet();
			if (iconset != nullptr)
			{
				_downButton.setText("");
				_downButton.setIcon(iconset, 5);
				_upButton.setText("");
				_upButton.setIcon(iconset, 4);
			}

			sizeHint().configureMinimalGenerator(
				[this]()
				{
					const spk::Vector2UInt downSize = _downButton.minimalSize();
					const spk::Vector2UInt editSize = _valueEdit.minimalSize();
					const spk::Vector2UInt upSize = _upButton.minimalSize();

					return spk::Vector2UInt(downSize.x + editSize.x + upSize.x, std::max({downSize.y, editSize.y, upSize.y}));
				});

			activate();
		}

		Contract subscribeToEdition(Callback p_callback)
		{
			return _value.subscribe(std::move(p_callback));
		}

		void setValue(TType p_value)
		{
			_value = _clampedValue(p_value);
		}

		[[nodiscard]] TType value() const
		{
			return _value.value();
		}

		void setStep(TType p_step)
		{
			_step = p_step;
		}

		[[nodiscard]] TType step() const
		{
			return _step;
		}

		void setMinimalLimit(TType p_minimalValue)
		{
			_minLimit = p_minimalValue;
			setValue(_value.value());
		}

		void setMaximalLimit(TType p_maximalValue)
		{
			_maxLimit = p_maximalValue;
			setValue(_value.value());
		}

		void setLimits(TType p_minimalValue, TType p_maximalValue)
		{
			setMinimalLimit(p_minimalValue);
			setMaximalLimit(p_maximalValue);
		}

		void removeLimits()
		{
			_minLimit.reset();
			_maxLimit.reset();
		}

		[[nodiscard]] const std::optional<TType>& minimalLimit() const
		{
			return _minLimit;
		}

		[[nodiscard]] const std::optional<TType>& maximalLimit() const
		{
			return _maxLimit;
		}

		[[nodiscard]] spk::PushButton& downButton()
		{
			return _downButton;
		}

		[[nodiscard]] const spk::PushButton& downButton() const
		{
			return _downButton;
		}

		[[nodiscard]] spk::PushButton& upButton()
		{
			return _upButton;
		}

		[[nodiscard]] const spk::PushButton& upButton() const
		{
			return _upButton;
		}

		[[nodiscard]] spk::TextEdit& valueEdit()
		{
			return _valueEdit;
		}

		[[nodiscard]] const spk::TextEdit& valueEdit() const
		{
			return _valueEdit;
		}
	};
}
