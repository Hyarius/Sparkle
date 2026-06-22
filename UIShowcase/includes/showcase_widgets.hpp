#pragma once

#include <algorithm>
#include <string>

#include <sparkle.hpp>

namespace showcase
{
	// Helper that builds a Rect2D from signed values, clamping the size to be non-negative.
	[[nodiscard]] inline spk::Rect2D makeRect(int p_x, int p_y, int p_width, int p_height)
	{
		const auto width = static_cast<std::size_t>(std::max(0, p_width));
		const auto height = static_cast<std::size_t>(std::max(0, p_height));
		return spk::Rect2D(p_x, p_y, width, height);
	}

	// A bare container widget that stretches every direct child to fill its own geometry.
	//
	// The showcase uses it as a swappable mount point: a page parents a single root widget
	// into the host, and FillHost keeps that root sized to the host regardless of when it
	// was attached (newly added children are sized through relayout()).
	class FillHost : public spk::Widget 
	{
	public:
		explicit FillHost(const std::string &p_name, spk::Widget *p_parent = nullptr) :
			spk::Widget(p_name, p_parent)
		{
			activate();
		}

		// Re-apply the fill geometry to every child. Needed when a child is attached while
		// the host already has its final geometry, since Widget::setGeometry is a no-op when
		// the geometry is unchanged and would not propagate to the freshly added child.
		void relayout()
		{
			const spk::Rect2D bounds = geometry().atOrigin();

			for (auto *child : children())
			{
				if (child != nullptr)
				{
					child->setGeometry(bounds);
				}
			}
		}

	protected:
		void _onGeometryChange() override
		{
			relayout();
		}
	};

	// A container widget that owns a vertical layout and drives it from its own geometry,
	// applying a uniform inner padding. Pages use it as a simple stacking root.
	class VerticalStack : public spk::Widget
	{
	private:
		spk::VerticalLayout _layout;
		spk::Vector2Int _padding = {16, 16};

	public:
		explicit VerticalStack(const std::string &p_name, spk::Widget *p_parent = nullptr) :
			spk::Widget(p_name, p_parent)
		{
			_layout.setElementPadding({8, 8});
			activate();
		}

		[[nodiscard]] spk::VerticalLayout &layout()
		{
			return _layout;
		}

		void setPadding(const spk::Vector2Int &p_padding)
		{
			_padding = p_padding;
			_onGeometryChange();
		}

	protected:
		void _onGeometryChange() override
		{
			const spk::Rect2D bounds = geometry().atOrigin();

			const int innerWidth = static_cast<int>(bounds.size.x) - (_padding.x * 2);
			const int innerHeight = static_cast<int>(bounds.size.y) - (_padding.y * 2);

			_layout.setGeometry(makeRect(bounds.anchor.x + _padding.x, bounds.anchor.y + _padding.y, innerWidth, innerHeight));
		}
	};

	// A nine-slice Panel that owns a layout and drives it from its own geometry, inset by a
	// uniform padding. This lets each showcase section position its children through a layout
	// instead of manual geometry, while still drawing a panel background.
	template <typename TLayout>
	class LayoutPanel : public spk::Panel
	{
	private:
		TLayout _layout;
		spk::Vector2Int _padding;

	public:
		LayoutPanel(
			const std::string &p_name,
			const spk::WidgetStyle &p_style,
			const spk::Vector2Int &p_padding,
			spk::Widget *p_parent = nullptr) :
			spk::Panel(p_name, p_style, p_parent),
			_padding(p_padding)
		{
		}

		[[nodiscard]] TLayout &layout()
		{
			return _layout;
		}

		void setInnerPadding(const spk::Vector2Int &p_padding)
		{
			_padding = p_padding;
			_onGeometryChange();
		}

	protected:
		void _onGeometryChange() override
		{
			const spk::Rect2D bounds = geometry().atOrigin();

			const int innerWidth = static_cast<int>(bounds.size.x) - (_padding.x * 2);
			const int innerHeight = static_cast<int>(bounds.size.y) - (_padding.y * 2);

			_layout.setGeometry(makeRect(bounds.anchor.x + _padding.x, bounds.anchor.y + _padding.y, innerWidth, innerHeight));
		}
	};
}
