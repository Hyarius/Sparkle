#pragma once

#include <cstdint>
#include <string>

#include "structures/math/spk_rect_2d.hpp"
#include "structures/widget/spk_widget.hpp"

namespace spk
{
	class ScalableWidget : public spk::Widget
	{
	protected:
		enum Edge : uint8_t
		{
			None = 0,
			Left = 1,
			Right = 2,
			Top = 4,
			Bottom = 8
		};

		static constexpr int EdgeGrabOffset = 5;

		uint8_t _activeEdges = Edge::None;
		uint8_t _hoveredEdges = Edge::None;

		spk::Rect2D _baseGeometry;
		spk::Vector2Int _resizeOrigin;

		[[nodiscard]] uint8_t _hoverEdges(const spk::Vector2Int &p_mousePosition) const;
		void _requestCursorForEdges(uint8_t p_edges, const spk::Mouse &p_mouse) const;
		void _beginResize(uint8_t p_edges, const spk::Vector2Int &p_startPosition);
		void _endResize();
		[[nodiscard]] spk::Rect2D _computeResizedGeometry(const spk::Vector2Int &p_currentPosition) const;

		void _onMouseMovedEvent(spk::MouseMovedEvent &p_event) override;
		void _onMouseButtonPressedEvent(spk::MouseButtonPressedEvent &p_event) override;
		void _onMouseButtonReleasedEvent(spk::MouseButtonReleasedEvent &p_event) override;

	public:
		explicit ScalableWidget(const std::string &p_name, spk::Widget *p_parent = nullptr);

		void setGeometry(const spk::Rect2D &p_geometry) override;

		void setMinimumSize(const spk::Vector2UInt &p_minimumSize);
		void setMaximumSize(const spk::Vector2UInt &p_maximumSize);

		[[nodiscard]] bool isResizing() const;
	};
}
