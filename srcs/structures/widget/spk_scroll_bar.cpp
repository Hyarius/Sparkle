#include "structures/widget/spk_scroll_bar.hpp"

#include <algorithm>
#include <stdexcept>
#include <utility>

#include "structures/widget/spk_widget_style.hpp"

namespace
{
	constexpr size_t UpIconID = 4;
	constexpr size_t DownIconID = 5;
	constexpr size_t LeftIconID = 6;
	constexpr size_t RightIconID = 7;

	[[nodiscard]] std::shared_ptr<spk::SpriteSheet> defaultIconset()
	{
		return spk::WidgetStyle::Collection::style(spk::WidgetStyle::Collection::Default).iconSpriteSheet();
	}
}

namespace spk
{
	ScrollBar::ScrollBar(const std::string &p_name, spk::Widget *p_parent) :
		ScrollBar(p_name, spk::Orientation::Horizontal, p_parent)
	{
	}

	ScrollBar::ScrollBar(
		const std::string &p_name,
		spk::Orientation p_orientation,
		spk::Widget *p_parent) :
		spk::Widget(p_name, p_parent),
		_negativeButton(p_name + "::negativeButton", this),
		_positiveButton(p_name + "::positiveButton", this),
		_sliderBar(p_name + "::sliderBar", p_orientation, this)
	{
		_negativeButtonContract = _negativeButton.subscribeToClick([this]() {
			setRatio(_sliderBar.ratio() - _step);
		});

		_positiveButtonContract = _positiveButton.subscribeToClick([this]() {
			setRatio(_sliderBar.ratio() + _step);
		});

		_sliderBarContract = _sliderBar.subscribeToEdition([this](float p_ratio) {
			_onEditionProvider.trigger(p_ratio);
		});

		_refreshButtonCaptions();
		activate();
	}

	void ScrollBar::_refreshButtonCaptions()
	{
		switch (_sliderBar.orientation())
		{
		case spk::Orientation::Horizontal:
			_negativeButton.setIcon(defaultIconset(), LeftIconID);
			_positiveButton.setIcon(defaultIconset(), RightIconID);
			break;
		case spk::Orientation::Vertical:
			_negativeButton.setIcon(defaultIconset(), UpIconID);
			_positiveButton.setIcon(defaultIconset(), DownIconID);
			break;
		}
	}

	void ScrollBar::_onGeometryChange()
	{
		switch (_sliderBar.orientation())
		{
		case spk::Orientation::Horizontal:
		{
			const unsigned int buttonSize = geometry().height();
			const unsigned int sliderWidth =
				(geometry().width() > buttonSize * 2) ? geometry().width() - buttonSize * 2 : 0;

			_negativeButton.setGeometry(spk::Rect2D(0, 0, buttonSize, buttonSize));
			_sliderBar.setGeometry(spk::Rect2D(static_cast<int>(buttonSize), 0, sliderWidth, geometry().height()));
			_positiveButton.setGeometry(spk::Rect2D(static_cast<int>(buttonSize + sliderWidth), 0, buttonSize, buttonSize));
			break;
		}
		case spk::Orientation::Vertical:
		{
			const unsigned int buttonSize = geometry().width();
			const unsigned int sliderHeight =
				(geometry().height() > buttonSize * 2) ? geometry().height() - buttonSize * 2 : 0;

			_negativeButton.setGeometry(spk::Rect2D(0, 0, buttonSize, buttonSize));
			_sliderBar.setGeometry(spk::Rect2D(0, static_cast<int>(buttonSize), geometry().width(), sliderHeight));
			_positiveButton.setGeometry(spk::Rect2D(0, static_cast<int>(buttonSize + sliderHeight), buttonSize, buttonSize));
			break;
		}
		}
	}

	ScrollBar::Contract ScrollBar::subscribeToEdition(Callback p_callback)
	{
		return _onEditionProvider.subscribe(std::move(p_callback));
	}

	void ScrollBar::setOrientation(spk::Orientation p_orientation)
	{
		_sliderBar.setOrientation(p_orientation);
		_refreshButtonCaptions();
		_onGeometryChange();
	}

	void ScrollBar::setScale(float p_scale)
	{
		_sliderBar.setScale(p_scale);
	}

	void ScrollBar::setRatio(float p_ratio)
	{
		_sliderBar.setRatio(p_ratio);
	}

	void ScrollBar::setStep(float p_step)
	{
		if (p_step <= 0.0f || p_step > 1.0f)
		{
			throw std::invalid_argument("ScrollBar step must belong to ]0, 1]");
		}

		_step = p_step;
	}

	spk::Orientation ScrollBar::orientation() const
	{
		return _sliderBar.orientation();
	}

	float ScrollBar::ratio() const
	{
		return _sliderBar.ratio();
	}

	float ScrollBar::step() const
	{
		return _step;
	}

	spk::PushButton &ScrollBar::negativeButton()
	{
		return _negativeButton;
	}

	const spk::PushButton &ScrollBar::negativeButton() const
	{
		return _negativeButton;
	}

	spk::PushButton &ScrollBar::positiveButton()
	{
		return _positiveButton;
	}

	const spk::PushButton &ScrollBar::positiveButton() const
	{
		return _positiveButton;
	}

	spk::SliderBar &ScrollBar::sliderBar()
	{
		return _sliderBar;
	}

	const spk::SliderBar &ScrollBar::sliderBar() const
	{
		return _sliderBar;
	}
}
