#pragma once

#include <algorithm>
#include <memory>
#include <string>

#include "page_support.hpp"
#include "showcase_page.hpp"

namespace showcase
{
	// A self-sizing scroll demo: it owns a vertical ScrollArea filled with a fixed list of rows,
	// and feeds the scroll area its content extent on every geometry change (the scroll area
	// cannot infer its scrollable size on its own, the same pattern the showcase shell uses).
	class ScrollHost : public spk::Widget
	{
	private:
		static constexpr int rowHeight = 30;
		static constexpr int rowSpacing = 6;
		static constexpr int contentPadding = 8;
		static constexpr int rowCount = 24;

		spk::ScrollArea<VerticalStack> _scroll;
		WidgetBag _items;

	public:
		explicit ScrollHost(const std::string &p_name, spk::Widget *p_parent = nullptr) :
			spk::Widget(p_name, p_parent),
			_scroll(p_name + "::scroll", this)
		{
			_scroll.setScrollBarVisible(spk::Orientation::Horizontal, false);

			VerticalStack &content = _scroll.contentObject();
			content.setPadding({contentPadding, contentPadding});
			content.layout().setElementPadding({0, rowSpacing});

			for (int index = 0; index < rowCount; ++index)
			{
				auto label = std::make_unique<spk::TextLabel>(
					p_name + "::item::" + std::to_string(index),
					"Scrollable item #" + std::to_string(index + 1),
					theme::bodyStyle(),
					&content);
				label->setAlignment(spk::HorizontalAlignment::Left, spk::VerticalAlignment::Centered);

				spk::Layout::Element *element = content.layout().addWidget(label.get(), spk::Layout::SizePolicy::Fixed);
				element->setSize(spk::Vector2UInt(0, static_cast<unsigned int>(rowHeight)));

				_items.push_back(std::move(label));
			}

			activate();
		}

	protected:
		void _onGeometryChange() override
		{
			_scroll.setGeometry(geometry().atOrigin());

			const int viewWidth = static_cast<int>(_scroll.viewSize().x);
			const int contentHeight =
				(rowCount * rowHeight) +
				(std::max(0, rowCount - 1) * rowSpacing) +
				(contentPadding * 2);

			_scroll.setContentSize(spk::Vector2UInt(
				static_cast<unsigned int>(std::max(0, viewWidth)),
				static_cast<unsigned int>(std::max(0, contentHeight))));
		}
	};

	// Showcases spk::ScrollArea: a clipped viewport over a list taller than the available space,
	// with a vertical scroll bar and mouse-wheel scrolling.
	class ScrollPage : public ShowcasePage
	{
	private:
		std::unique_ptr<VerticalStack> _contentRoot;
		WidgetBag _contentBag;
		std::unique_ptr<VerticalStack> _propertiesRoot;
		WidgetBag _propertiesBag;

	public:
		[[nodiscard]] std::string_view title() const override;
		[[nodiscard]] std::string_view description() const override;

		void buildContent(spk::Widget &p_parent) override;
		void buildProperties(spk::Widget &p_parent) override;
	};
}
