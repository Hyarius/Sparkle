#include "structures/widget/spk_scroll_area.hpp"

#include <algorithm>

namespace spk
{
	IScrollArea::IScrollArea(const std::string& p_name, spk::Widget* p_parent) :
		spk::Widget(p_name, p_parent),
		_container(p_name + "::container", this),
		_horizontalScrollBar(p_name + "::horizontalScrollBar", spk::Orientation::Horizontal, this),
		_verticalScrollBar(p_name + "::verticalScrollBar", spk::Orientation::Vertical, this)
	{
		_horizontalBarContract = _horizontalScrollBar.subscribeToEdition([this](float)
		{
			_refreshContentAnchor();
		});

		_verticalBarContract = _verticalScrollBar.subscribeToEdition([this](float)
		{
			_refreshContentAnchor();
		});

		activate();
	}

	spk::Vector2UInt IScrollArea::viewSize() const
	{
		unsigned int width = geometry().width();
		unsigned int height = geometry().height();

		if (_verticalScrollBar.isActivated() == true)
		{
			width = (width > _scrollBarWidth) ? width - _scrollBarWidth : 0;
		}

		if (_horizontalScrollBar.isActivated() == true)
		{
			height = (height > _scrollBarWidth) ? height - _scrollBarWidth : 0;
		}

		return {width, height};
	}

	void IScrollArea::_refreshContentAnchor()
	{
		const spk::Vector2UInt view = viewSize();
		const spk::Vector2UInt content = _container.contentSize();

		const int horizontalOverflow =
			(content.x > view.x) ? static_cast<int>(content.x - view.x) : 0;
		const int verticalOverflow =
			(content.y > view.y) ? static_cast<int>(content.y - view.y) : 0;

		_container.setContentAnchor({
			-static_cast<int>(static_cast<float>(horizontalOverflow) * _horizontalScrollBar.ratio()),
			-static_cast<int>(static_cast<float>(verticalOverflow) * _verticalScrollBar.ratio())});
	}

	void IScrollArea::_refreshScrollBarScales()
	{
		const spk::Vector2UInt view = viewSize();
		const spk::Vector2UInt content = _container.contentSize();

		const float horizontalScale =
			(content.x > view.x && content.x > 0)
				? static_cast<float>(view.x) / static_cast<float>(content.x)
				: 1.0f;
		const float verticalScale =
			(content.y > view.y && content.y > 0)
				? static_cast<float>(view.y) / static_cast<float>(content.y)
				: 1.0f;

		_horizontalScrollBar.setScale(std::clamp(horizontalScale, 0.05f, 1.0f));
		_verticalScrollBar.setScale(std::clamp(verticalScale, 0.05f, 1.0f));
	}

	void IScrollArea::_onGeometryChange()
	{
		const spk::Vector2UInt view = viewSize();

		_container.setGeometry(spk::Rect2D(0, 0, view.x, view.y));

		_horizontalScrollBar.setGeometry(spk::Rect2D(
			0,
			static_cast<int>(view.y),
			view.x,
			(_horizontalScrollBar.isActivated() == true) ? _scrollBarWidth : 0));

		_verticalScrollBar.setGeometry(spk::Rect2D(
			static_cast<int>(view.x),
			0,
			(_verticalScrollBar.isActivated() == true) ? _scrollBarWidth : 0,
			view.y));

		_refreshScrollBarScales();
		_refreshContentAnchor();
	}

	void IScrollArea::_onMouseWheelScrolledEvent(spk::MouseWheelScrolledEvent& p_event)
	{
		if (absoluteGeometry().contains(p_event.device().position) == false)
		{
			return;
		}

		if (_verticalScrollBar.isActivated() == true)
		{
			_verticalScrollBar.setRatio(_verticalScrollBar.ratio() - p_event->delta.y * _verticalScrollBar.step());
			p_event.consume();
		}
	}

	void IScrollArea::setContent(spk::Widget* p_content)
	{
		_container.setContent(p_content);
	}

	void IScrollArea::setContentSize(const spk::Vector2UInt& p_contentSize)
	{
		_container.setContentSize(p_contentSize);
		_refreshScrollBarScales();
		_refreshContentAnchor();
	}

	void IScrollArea::setScrollBarVisible(spk::Orientation p_orientation, bool p_state)
	{
		spk::ScrollBar& bar =
			(p_orientation == spk::Orientation::Horizontal) ? _horizontalScrollBar : _verticalScrollBar;

		if (p_state == true)
		{
			bar.activate();
		}
		else
		{
			bar.deactivate();
		}

		_onGeometryChange();
	}

	void IScrollArea::setScrollBarWidth(unsigned int p_scrollBarWidth)
	{
		if (_scrollBarWidth == p_scrollBarWidth)
		{
			return;
		}

		_scrollBarWidth = p_scrollBarWidth;
		_onGeometryChange();
	}

	spk::Widget* IScrollArea::content()
	{
		return _container.content();
	}

	const spk::Widget* IScrollArea::content() const
	{
		return _container.content();
	}

	const spk::Vector2UInt& IScrollArea::contentSize() const
	{
		return _container.contentSize();
	}

	unsigned int IScrollArea::scrollBarWidth() const
	{
		return _scrollBarWidth;
	}

	spk::ContainerWidget& IScrollArea::container()
	{
		return _container;
	}

	const spk::ContainerWidget& IScrollArea::container() const
	{
		return _container;
	}

	spk::ScrollBar& IScrollArea::horizontalScrollBar()
	{
		return _horizontalScrollBar;
	}

	const spk::ScrollBar& IScrollArea::horizontalScrollBar() const
	{
		return _horizontalScrollBar;
	}

	spk::ScrollBar& IScrollArea::verticalScrollBar()
	{
		return _verticalScrollBar;
	}

	const spk::ScrollBar& IScrollArea::verticalScrollBar() const
	{
		return _verticalScrollBar;
	}
}
