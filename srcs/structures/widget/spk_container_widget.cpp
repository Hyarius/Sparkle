#include "structures/widget/spk_container_widget.hpp"

#include <stdexcept>

namespace spk
{
	ContainerWidget::ContainerWidget(const std::string &p_name, spk::Widget *p_parent) :
		spk::Widget(p_name, p_parent)
	{
		activate();
	}

	void ContainerWidget::_refreshContentGeometry()
	{
		if (_content == nullptr)
		{
			return;
		}

		_content->setGeometry(spk::Rect2D(_contentAnchor, _contentSize));
	}

	void ContainerWidget::_onGeometryChange()
	{
		_refreshContentGeometry();
	}

	void ContainerWidget::setContent(spk::Widget *p_content)
	{
		if (p_content != nullptr && p_content->parent() != this)
		{
			throw std::invalid_argument("ContainerWidget content must be one of its children");
		}

		_content = p_content;
		_refreshContentGeometry();
	}

	void ContainerWidget::setContentAnchor(const spk::Vector2Int &p_contentAnchor)
	{
		if (_contentAnchor == p_contentAnchor)
		{
			return;
		}

		_contentAnchor = p_contentAnchor;
		_refreshContentGeometry();
	}

	void ContainerWidget::setContentSize(const spk::Vector2UInt &p_contentSize)
	{
		if (_contentSize == p_contentSize)
		{
			return;
		}

		_contentSize = p_contentSize;
		_refreshContentGeometry();
	}

	spk::Widget *ContainerWidget::content()
	{
		return _content;
	}

	const spk::Widget *ContainerWidget::content() const
	{
		return _content;
	}

	const spk::Vector2Int &ContainerWidget::contentAnchor() const
	{
		return _contentAnchor;
	}

	const spk::Vector2UInt &ContainerWidget::contentSize() const
	{
		return _contentSize;
	}
}
