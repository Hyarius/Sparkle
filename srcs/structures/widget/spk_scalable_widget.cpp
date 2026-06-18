#include "structures/widget/spk_scalable_widget.hpp"

#include <algorithm>
#include <limits>

namespace
{
	void compute1DResize(
		int p_baseAnchor, int p_baseSize, int p_delta, int p_minSize, int p_maxSize, bool p_fromMinSide, int& p_outAnchor, int& p_outSize)
	{
		if (p_fromMinSide == true)
		{
			const int proposed = std::clamp(p_baseSize - p_delta, p_minSize, p_maxSize);

			p_outAnchor = p_baseAnchor + (p_baseSize - proposed);
			p_outSize = (p_baseAnchor + p_baseSize) - p_outAnchor;
		}
		else
		{
			p_outAnchor = p_baseAnchor;
			p_outSize = std::clamp(p_baseSize + p_delta, p_minSize, p_maxSize);
		}
	}
}

namespace spk
{
	ScalableWidget::ScalableWidget(const std::string& p_name, spk::Widget* p_parent) :
		spk::Widget(p_name, p_parent)
	{
		setMaximalSize({1000, 1000});
	}

	void ScalableWidget::setGeometry(const spk::Rect2D& p_geometry)
	{
		const spk::Vector2UInt minimumSize = spk::Widget::minimalSize();
		const spk::Vector2UInt maximumSize = spk::Widget::maximalSize();

		spk::Rect2D clamped = p_geometry;
		clamped.size.x = std::clamp(clamped.size.x, minimumSize.x, maximumSize.x);
		clamped.size.y = std::clamp(clamped.size.y, minimumSize.y, maximumSize.y);

		spk::Widget::setGeometry(clamped);
	}

	void ScalableWidget::setMinimumSize(const spk::Vector2UInt& p_minimumSize)
	{
		setMinimalSize(p_minimumSize);

		if (geometry().size.x < p_minimumSize.x || geometry().size.y < p_minimumSize.y)
		{
			setGeometry(
				spk::Rect2D(
					geometry().anchor, spk::Vector2UInt(std::max(geometry().size.x, p_minimumSize.x), std::max(geometry().size.y, p_minimumSize.y))));
		}
	}

	void ScalableWidget::setMaximumSize(const spk::Vector2UInt& p_maximumSize)
	{
		setMaximalSize(p_maximumSize);
	}

	bool ScalableWidget::isResizing() const
	{
		return _activeEdges != Edge::None;
	}

	uint8_t ScalableWidget::_hoverEdges(const spk::Vector2Int& p_mousePosition) const
	{
		const spk::Rect2D& absolute = absoluteGeometry();

		const spk::Rect2D topArea(
			{absolute.anchor.x - EdgeGrabOffset, absolute.anchor.y - EdgeGrabOffset}, {absolute.size.x + EdgeGrabOffset * 2, EdgeGrabOffset * 2});
		const spk::Rect2D bottomArea(
			{absolute.anchor.x - EdgeGrabOffset, absolute.anchor.y + static_cast<int>(absolute.size.y) - EdgeGrabOffset},
			{absolute.size.x + EdgeGrabOffset * 2, EdgeGrabOffset * 2});
		const spk::Rect2D leftArea(
			{absolute.anchor.x - EdgeGrabOffset, absolute.anchor.y - EdgeGrabOffset}, {EdgeGrabOffset * 2, absolute.size.y + EdgeGrabOffset * 2});
		const spk::Rect2D rightArea(
			{absolute.anchor.x + static_cast<int>(absolute.size.x) - EdgeGrabOffset, absolute.anchor.y - EdgeGrabOffset},
			{EdgeGrabOffset * 2, absolute.size.y + EdgeGrabOffset * 2});

		uint8_t edges = Edge::None;
		if (topArea.contains(p_mousePosition) == true)
		{
			edges |= Edge::Top;
		}
		if (bottomArea.contains(p_mousePosition) == true)
		{
			edges |= Edge::Bottom;
		}
		if (leftArea.contains(p_mousePosition) == true)
		{
			edges |= Edge::Left;
		}
		if (rightArea.contains(p_mousePosition) == true)
		{
			edges |= Edge::Right;
		}
		return edges;
	}

	void ScalableWidget::_requestCursorForEdges(uint8_t p_edges, const spk::Mouse& p_mouse) const
	{
		const bool top = (p_edges & Edge::Top) != 0;
		const bool bottom = (p_edges & Edge::Bottom) != 0;
		const bool left = (p_edges & Edge::Left) != 0;
		const bool right = (p_edges & Edge::Right) != 0;

		if ((top && left) || (bottom && right))
		{
			p_mouse.requestCursor("ResizeNWSE");
		}
		else if ((top && right) || (bottom && left))
		{
			p_mouse.requestCursor("ResizeNESW");
		}
		else if (top || bottom)
		{
			p_mouse.requestCursor("ResizeNS");
		}
		else if (left || right)
		{
			p_mouse.requestCursor("ResizeWE");
		}
		else
		{
			p_mouse.requestCursor("Arrow");
		}
	}

	void ScalableWidget::_beginResize(uint8_t p_edges, const spk::Vector2Int& p_startPosition)
	{
		_activeEdges = p_edges;
		_resizeOrigin = p_startPosition;
		_baseGeometry = geometry();
	}

	void ScalableWidget::_endResize()
	{
		_activeEdges = Edge::None;
	}

	spk::Rect2D ScalableWidget::_computeResizedGeometry(const spk::Vector2Int& p_currentPosition) const
	{
		const spk::Vector2Int delta = p_currentPosition - _resizeOrigin;
		const spk::Vector2UInt minimumSize = spk::Widget::minimalSize();
		const spk::Vector2UInt maximumSize = spk::Widget::maximalSize();

		int anchorX = _baseGeometry.anchor.x;
		int anchorY = _baseGeometry.anchor.y;
		int sizeX = static_cast<int>(_baseGeometry.size.x);
		int sizeY = static_cast<int>(_baseGeometry.size.y);

		if ((_activeEdges & Edge::Top) != 0)
		{
			compute1DResize(
				_baseGeometry.anchor.y,
				static_cast<int>(_baseGeometry.size.y),
				delta.y,
				static_cast<int>(minimumSize.y),
				static_cast<int>(std::min<uint32_t>(maximumSize.y, static_cast<uint32_t>(std::numeric_limits<int>::max()))),
				true,
				anchorY,
				sizeY);
		}
		else if ((_activeEdges & Edge::Bottom) != 0)
		{
			compute1DResize(
				_baseGeometry.anchor.y,
				static_cast<int>(_baseGeometry.size.y),
				delta.y,
				static_cast<int>(minimumSize.y),
				static_cast<int>(std::min<uint32_t>(maximumSize.y, static_cast<uint32_t>(std::numeric_limits<int>::max()))),
				false,
				anchorY,
				sizeY);
		}

		if ((_activeEdges & Edge::Left) != 0)
		{
			compute1DResize(
				_baseGeometry.anchor.x,
				static_cast<int>(_baseGeometry.size.x),
				delta.x,
				static_cast<int>(minimumSize.x),
				static_cast<int>(std::min<uint32_t>(maximumSize.x, static_cast<uint32_t>(std::numeric_limits<int>::max()))),
				true,
				anchorX,
				sizeX);
		}
		else if ((_activeEdges & Edge::Right) != 0)
		{
			compute1DResize(
				_baseGeometry.anchor.x,
				static_cast<int>(_baseGeometry.size.x),
				delta.x,
				static_cast<int>(minimumSize.x),
				static_cast<int>(std::min<uint32_t>(maximumSize.x, static_cast<uint32_t>(std::numeric_limits<int>::max()))),
				false,
				anchorX,
				sizeX);
		}

		return spk::Rect2D(anchorX, anchorY, static_cast<unsigned int>(std::max(0, sizeX)), static_cast<unsigned int>(std::max(0, sizeY)));
	}

	void ScalableWidget::_onMouseMovedEvent(spk::MouseMovedEvent& p_event)
	{
		if (_activeEdges == Edge::None)
		{
			const uint8_t hovered = _hoverEdges(p_event.device().position);
			if (hovered != _hoveredEdges)
			{
				_hoveredEdges = hovered;
				_requestCursorForEdges(hovered, p_event.device());
			}
			return;
		}

		const spk::Rect2D newGeometry = _computeResizedGeometry(p_event.device().position);
		if (newGeometry != geometry())
		{
			setGeometry(newGeometry);
			p_event.consume();
		}
	}

	void ScalableWidget::_onMouseButtonPressedEvent(spk::MouseButtonPressedEvent& p_event)
	{
		if (p_event->button != spk::Mouse::Left)
		{
			return;
		}

		const uint8_t edges = _hoverEdges(p_event.device().position);
		if (edges != Edge::None)
		{
			_beginResize(edges, p_event.device().position);
			takeFocus(FocusType::Mouse);
			p_event.consume();
		}
	}

	void ScalableWidget::_onMouseButtonReleasedEvent(spk::MouseButtonReleasedEvent& p_event)
	{
		if (p_event->button != spk::Mouse::Left || _activeEdges == Edge::None)
		{
			return;
		}

		_endResize();
		releaseFocus(FocusType::Mouse);
		p_event.device().requestCursor("Arrow");
		p_event.consume();
	}
}
