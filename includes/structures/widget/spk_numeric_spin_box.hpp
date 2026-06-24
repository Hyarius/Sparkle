#pragma once

#include <algorithm>
#include <charconv>
#include <sstream>
#include <string>
#include <type_traits>

#include "structures/container/spk_observable_value.hpp"
#include "structures/widget/spk_icon_button.hpp"
#include "structures/widget/spk_text_edit.hpp"
#include "structures/widget/spk_widget.hpp"

namespace spk
{
	template <typename TType>
		requires std::is_arithmetic_v<TType>
	class NumericSpinBox : public spk::Widget
	{
	public:
		using value_type = TType;

		using ValidationState = spk::TextEdit::ValidationState;
		using EditionCallback = spk::ObservableValue<value_type>::Callback;
		using EditionContract = spk::ObservableValue<value_type>::Contract;

	private:
		static constexpr size_t UpIconID = 4;
		static constexpr size_t DownIconID = 5;

		spk::TextEdit _valueEdit;
		spk::IconButton _lowerButton;
		spk::IconButton _raiseButton;

		spk::TextEdit::EditionContract _onTextEditionContract;
		spk::PushButton::Contract _raiseContract;
		spk::PushButton::Contract _lowerContract;

		spk::ObservableValue<value_type> _value{static_cast<value_type>(0)};
		value_type _step{static_cast<value_type>(1)};
		bool _isRefreshingText = false;

		[[nodiscard]] static bool _isEmptyOrSignOnly(const std::string &p_text)
		{
			if (p_text.empty() == true || p_text == "-" || p_text == "+")
			{
				return true;
			}

			if constexpr (std::is_floating_point_v<value_type>)
			{
				if (p_text == "." || p_text == "-." || p_text == "+.")
				{
					return true;
				}
			}

			return false;
		}

		[[nodiscard]] static ValidationState _parseValue(const std::string &p_text, value_type &p_outValue)
		{
			if constexpr (std::is_unsigned_v<value_type>)
			{
				if (p_text.empty() == false && p_text.front() == '-')
				{
					return ValidationState::Invalid;
				}
			}

			if (_isEmptyOrSignOnly(p_text) == true)
			{
				return ValidationState::Undefined;
			}

			if constexpr (std::is_integral_v<value_type>)
			{
				const char *begin = p_text.data();
				const char *end = p_text.data() + p_text.size();
				const char *parseBegin = (p_text.front() == '+') ? begin + 1 : begin;

				value_type value{};
				auto [ptr, ec] = std::from_chars(parseBegin, end, value, 10);
				if (ec != std::errc() || ptr != end)
				{
					return ValidationState::Invalid;
				}

				p_outValue = value;
				return ValidationState::Valid;
			}
			else
			{
				std::istringstream stream(p_text);
				value_type value{};
				stream >> value;

				if (stream.fail() == true)
				{
					return ValidationState::Invalid;
				}

				stream >> std::ws;
				if (stream.eof() == false)
				{
					return ValidationState::Invalid;
				}

				p_outValue = value;
				return ValidationState::Valid;
			}
		}

		[[nodiscard]] static std::string _formatValue(const value_type &p_value)
		{
			return std::to_string(p_value);
		}

		void _refreshValueText()
		{
			_isRefreshingText = true;
			_valueEdit.setText(_formatValue(_value.value()));
			_isRefreshingText = false;
		}

		void _syncValueFromTextIfValid()
		{
			if (_isRefreshingText == true)
			{
				return;
			}

			value_type parsedValue{};
			if (_parseValue(_valueEdit.textAsUTF8(), parsedValue) == ValidationState::Valid)
			{
				_value = parsedValue;
			}
		}

	protected:
		void _onGeometryChange() override
		{
			const unsigned int buttonSize = geometry().height();
			const unsigned int editWidth =
				(geometry().width() > buttonSize * 2) ? geometry().width() - buttonSize * 2 : 0;

			_valueEdit.setGeometry(spk::Rect2D(0, 0, editWidth, geometry().height()));
			_lowerButton.setGeometry(spk::Rect2D(static_cast<int>(editWidth), 0, buttonSize, buttonSize));
			_raiseButton.setGeometry(spk::Rect2D(static_cast<int>(editWidth + buttonSize), 0, buttonSize, buttonSize));
		}

	public:
		explicit NumericSpinBox(const std::string &p_name, spk::Widget *p_parent = nullptr) :
			spk::Widget(p_name, p_parent),
			_valueEdit(p_name + "::valueEdit", this),
			_lowerButton(p_name + "::lowerButton", this),
			_raiseButton(p_name + "::raiseButton", this)
		{
			_lowerButton.setIconSpriteID(DownIconID);
			_raiseButton.setIconSpriteID(UpIconID);

			_valueEdit.setPlaceholder("...");
			_valueEdit.setValidationCallback(
				[](const spk::Font::Text &p_text) {
					std::string utf8;
					utf8.reserve(p_text.size());
					for (char32_t codepoint : p_text)
					{
						if (codepoint > 127)
						{
							return ValidationState::Invalid;
						}
						utf8.push_back(static_cast<char>(codepoint));
					}

					value_type ignored{};
					return _parseValue(utf8, ignored);
				});

			_onTextEditionContract = _valueEdit.subscribeToEdition([this](const spk::Font::Text &) {
				_syncValueFromTextIfValid();
			});

			_raiseContract = _raiseButton.subscribeToClick([this]() {
				increase();
			});

			_lowerContract = _lowerButton.subscribeToClick([this]() {
				decrease();
			});

			_refreshValueText();

			configureMinimalSizeGenerator([this]() {
				const spk::Vector2UInt editSize = _valueEdit.minimalSize();
				const spk::Vector2UInt lowerSize = _lowerButton.minimalSize();
				const spk::Vector2UInt raiseSize = _raiseButton.minimalSize();

				return spk::Vector2UInt(
					editSize.x + lowerSize.x + raiseSize.x,
					std::max({editSize.y, lowerSize.y, raiseSize.y}));
			});

			activate();
		}

		EditionContract subscribeToEdition(EditionCallback p_callback)
		{
			return _value.subscribe(std::move(p_callback));
		}

		void setValue(const value_type &p_value)
		{
			_value = p_value;
			_refreshValueText();
		}

		[[nodiscard]] const value_type &value() const
		{
			return _value.value();
		}

		void setStep(const value_type &p_step)
		{
			_step = p_step;
		}

		[[nodiscard]] const value_type &step() const
		{
			return _step;
		}

		void increase()
		{
			setValue(static_cast<value_type>(_value.value() + _step));
		}

		void decrease()
		{
			setValue(static_cast<value_type>(_value.value() - _step));
		}

		[[nodiscard]] spk::TextEdit &valueEdit()
		{
			return _valueEdit;
		}

		[[nodiscard]] const spk::TextEdit &valueEdit() const
		{
			return _valueEdit;
		}

		[[nodiscard]] spk::IconButton &raiseButton()
		{
			return _raiseButton;
		}

		[[nodiscard]] const spk::IconButton &raiseButton() const
		{
			return _raiseButton;
		}

		[[nodiscard]] spk::IconButton &lowerButton()
		{
			return _lowerButton;
		}

		[[nodiscard]] const spk::IconButton &lowerButton() const
		{
			return _lowerButton;
		}
	};

	using FloatSpinBox = NumericSpinBox<float>;
	using IntSpinBox = NumericSpinBox<int>;
	using UnsignedIntSpinBox = NumericSpinBox<unsigned int>;
}
