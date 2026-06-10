#pragma once

#include <string>
#include <type_traits>

#include "structures/math/spk_vector2.hpp"
#include "structures/widget/spk_container_widget.hpp"
#include "structures/widget/spk_scroll_bar.hpp"
#include "structures/widget/spk_widget.hpp"

namespace spk
{
	class IScrollArea : public spk::Widget
	{
	private:
		spk::ContainerWidget _container;
		spk::ScrollBar _horizontalScrollBar;
		spk::ScrollBar::Contract _horizontalBarContract;
		spk::ScrollBar _verticalScrollBar;
		spk::ScrollBar::Contract _verticalBarContract;

		unsigned int _scrollBarWidth = 16;

		void _refreshContentAnchor();
		void _refreshScrollBarScales();

	protected:
		void _onGeometryChange() override;
		void _onMouseWheelScrolledEvent(spk::MouseWheelScrolledEvent& p_event) override;

	public:
		explicit IScrollArea(const std::string& p_name, spk::Widget* p_parent = nullptr);

		void setContent(spk::Widget* p_content);
		void setContentSize(const spk::Vector2UInt& p_contentSize);
		void setScrollBarVisible(spk::Orientation p_orientation, bool p_state);
		void setScrollBarWidth(unsigned int p_scrollBarWidth);

		[[nodiscard]] spk::Widget* content();
		[[nodiscard]] const spk::Widget* content() const;
		[[nodiscard]] const spk::Vector2UInt& contentSize() const;
		[[nodiscard]] unsigned int scrollBarWidth() const;
		[[nodiscard]] spk::Vector2UInt viewSize() const;

		[[nodiscard]] spk::ContainerWidget& container();
		[[nodiscard]] const spk::ContainerWidget& container() const;
		[[nodiscard]] spk::ScrollBar& horizontalScrollBar();
		[[nodiscard]] const spk::ScrollBar& horizontalScrollBar() const;
		[[nodiscard]] spk::ScrollBar& verticalScrollBar();
		[[nodiscard]] const spk::ScrollBar& verticalScrollBar() const;
	};

	template <typename TContentType>
		requires std::is_base_of_v<spk::Widget, TContentType>
	class ScrollArea : public spk::IScrollArea
	{
	private:
		TContentType _contentObject;

		using spk::IScrollArea::setContent;

	public:
		explicit ScrollArea(const std::string& p_name, spk::Widget* p_parent = nullptr) :
			spk::IScrollArea(p_name, p_parent),
			_contentObject(p_name + "::content", &container())
		{
			setContent(&_contentObject);
			_contentObject.activate();
		}

		[[nodiscard]] TContentType& contentObject()
		{
			return _contentObject;
		}

		[[nodiscard]] const TContentType& contentObject() const
		{
			return _contentObject;
		}
	};
}
