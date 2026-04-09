#include "spk_widget.hpp"

namespace spk
{
	void Widget::_onGeometryUpdate()
	{
	}

	void Widget::_onRender()
	{
	}

	void Widget::_onUpdate(const spk::UpdateTick& p_tick)
	{
		(void)p_tick;
	}

	void Widget::_onFrameEvent(const spk::Event& p_event)
	{
		(void)p_event;
	}

	void Widget::_onMouseEvent(const spk::Event& p_event)
	{
		(void)p_event;
	}

	void Widget::_onKeyboardEvent(const spk::Event& p_event)
	{
		(void)p_event;
	}

	void Widget::_propagateFrameEvent(const spk::Event& p_event)
	{
		if (isActivated() == false)
		{
			return;
		}

		for (auto* child : children())
		{
			if (child != nullptr)
			{
				child->_propagateFrameEvent(p_event);

				if (p_event.metadata.isConsumed == true)
				{
					return;
				}
			}
		}

		_onFrameEvent(p_event);
	}

	void Widget::_propagateMouseEvent(const spk::Event& p_event)
	{
		if (isActivated() == false)
		{
			return;
		}

		for (auto* child : children())
		{
			if (child != nullptr)
			{
				child->_propagateMouseEvent(p_event);

				if (p_event.metadata.isConsumed == true)
				{
					return;
				}
			}
		}

		_onMouseEvent(p_event);
	}

	void Widget::_propagateKeyboardEvent(const spk::Event& p_event)
	{
		if (isActivated() == false)
		{
			return;
		}

		for (auto* child : children())
		{
			if (child != nullptr)
			{
				child->_propagateKeyboardEvent(p_event);

				if (p_event.metadata.isConsumed == true)
				{
					return;
				}
			}
		}

		_onKeyboardEvent(p_event);
	}

	void Widget::_updateGeometry()
	{
		if (_geometryChangeRequested == true)
		{
			updateGeometry();
		}
	}

	Widget::Widget(const std::string& p_name, spk::Widget* p_parent) :
		spk::InherenceTrait<Widget>(p_parent),
		_name(p_name)
	{
	}

	Widget::~Widget() = default;

	const std::string& Widget::name() const
	{
		return _name;
	}

	void Widget::setGeometry(const spk::Rect2D& p_geometry)
	{
		if (_geometry == p_geometry)
		{
			return;
		}

		_geometry = p_geometry;
		_geometryChangeRequested = true;
	}

	void Widget::requireGeometryUpdate()
	{
		_geometryChangeRequested = true;
	}

	void Widget::updateGeometry()
	{
		_onGeometryUpdate();
		_geometryChangeRequested = false;
	}

	const spk::Rect2D& Widget::geometry() const
	{
		return _geometry;
	}

	void Widget::render()
	{
		if (isActivated() == false)
		{
			return;
		}

		_updateGeometry();
		_onRender();

		for (auto* child : children())
		{
			if (child != nullptr)
			{
				child->render();
			}
		}
	}

	void Widget::update(const spk::UpdateTick& p_tick)
	{
		if (isActivated() == false)
		{
			return;
		}

		for (auto* child : children())
		{
			if (child != nullptr)
			{
				child->update(p_tick);
			}
		}

		_onUpdate(p_tick);
	}

	void Widget::dispatchFrameEvent(const spk::Event& p_event)
	{
		_propagateFrameEvent(p_event);
	}

	void Widget::dispatchMouseEvent(const spk::Event& p_event)
	{
		_propagateMouseEvent(p_event);
	}

	void Widget::dispatchKeyboardEvent(const spk::Event& p_event)
	{
		_propagateKeyboardEvent(p_event);
	}
}
