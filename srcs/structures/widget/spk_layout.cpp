#include "structures/widget/spk_layout.hpp"

#include <limits>
#include <stdexcept>

namespace spk
{
	Layout::Element::Element() = default;

	Layout::Element::Element(spk::Widget *p_widget, SizePolicy p_sizePolicy, const spk::Vector2UInt &p_size) :
		_widget(p_widget),
		_layout(nullptr),
		_sizePolicy(p_sizePolicy),
		_size(p_size)
	{
	}

	Layout::Element::Element(spk::Layout *p_layout, SizePolicy p_sizePolicy, const spk::Vector2UInt &p_size) :
		_widget(nullptr),
		_layout(p_layout),
		_sizePolicy(p_sizePolicy),
		_size(p_size)
	{
	}

	spk::Widget *Layout::Element::widget() const
	{
		return _widget;
	}

	spk::Layout *Layout::Element::layout() const
	{
		return _layout;
	}

	bool Layout::Element::isWidget() const
	{
		return _widget != nullptr;
	}

	bool Layout::Element::isLayout() const
	{
		return _layout != nullptr;
	}

	spk::Vector2UInt Layout::Element::minimalSize() const
	{
		if (_widget != nullptr)
		{
			return _widget->minimalSize();
		}
		if (_layout != nullptr)
		{
			return _layout->minimalSize();
		}
		return {0u, 0u};
	}

	spk::Vector2UInt Layout::Element::preferredSizeFor(const spk::Vector2UInt &p_availableSize) const
	{
		if (_widget != nullptr)
		{
			return _widget->preferredSizeFor(p_availableSize);
		}
		if (_layout != nullptr)
		{
			return _layout->preferredSizeFor(p_availableSize);
		}
		return {0u, 0u};
	}

	spk::Vector2UInt Layout::Element::fixedSize() const
	{
		if (_widget != nullptr)
		{
			return _widget->fixedSize();
		}
		if (_layout != nullptr)
		{
			return _layout->fixedSize();
		}
		return {0u, 0u};
	}

	spk::Vector2UInt Layout::Element::maximalSize() const
	{
		if (_widget != nullptr)
		{
			return _widget->maximalSize();
		}
		if (_layout != nullptr)
		{
			return _layout->maximalSize();
		}
		return {0u, 0u};
	}

	void Layout::Element::setGeometry(const spk::Rect2D &p_geometry)
	{
		if (_widget != nullptr)
		{
			_widget->setGeometry(p_geometry);
		}
		else if (_layout != nullptr)
		{
			_layout->setGeometry(p_geometry);
		}
	}

	void Layout::Element::setSizePolicy(SizePolicy p_sizePolicy)
	{
		_sizePolicy = p_sizePolicy;
	}

	Layout::SizePolicy Layout::Element::sizePolicy() const
	{
		return _sizePolicy;
	}

	void Layout::Element::setSize(const spk::Vector2UInt &p_size)
	{
		_size = p_size;
	}

	const spk::Vector2UInt &Layout::Element::size() const
	{
		return _size;
	}

	spk::Vector2UInt Layout::minimalSize() const
	{
		// Layouts have no invalidation hook on their children, so refresh on query.
		releaseMinimalSize();
		return spk::ResizableElement::minimalSize();
	}

	spk::Vector2UInt Layout::fixedSize() const
	{
		releaseFixedSize();
		return spk::ResizableElement::fixedSize();
	}

	spk::Vector2UInt Layout::maximalSize() const
	{
		releaseMaximalSize();
		return spk::ResizableElement::maximalSize();
	}

	void Layout::clear()
	{
		_elements.clear();
		releaseSizeCache();
	}

	Layout::Element *Layout::addWidget(spk::Widget *p_widget, SizePolicy p_sizePolicy)
	{
		if (p_widget == nullptr)
		{
			throw std::invalid_argument("Layout cannot hold a null widget");
		}

		_elements.push_back(std::make_unique<Element>(p_widget, p_sizePolicy, spk::Vector2UInt{0, 0}));
		releaseSizeCache();
		return _elements.back().get();
	}

	Layout::Element *Layout::addLayout(spk::Layout *p_layout, SizePolicy p_sizePolicy)
	{
		if (p_layout == nullptr)
		{
			throw std::invalid_argument("Layout cannot hold a null layout");
		}

		_elements.push_back(std::make_unique<Element>(p_layout, p_sizePolicy, spk::Vector2UInt{0, 0}));
		releaseSizeCache();
		return _elements.back().get();
	}

	void Layout::removeElement(Element *p_element)
	{
		if (p_element == nullptr)
		{
			return;
		}

		if (p_element->isWidget() == true)
		{
			removeWidget(p_element->widget());
		}
		else if (p_element->isLayout() == true)
		{
			removeLayout(p_element->layout());
		}
	}

	void Layout::removeWidget(spk::Widget *p_widget)
	{
		if (p_widget == nullptr)
		{
			return;
		}

		for (auto it = _elements.begin(); it != _elements.end(); ++it)
		{
			if (*it != nullptr && (*it)->widget() == p_widget)
			{
				_elements.erase(it);
				releaseSizeCache();
				return;
			}
		}

		throw std::runtime_error("Widget [" + p_widget->name() + "] not contained by the layout");
	}

	void Layout::removeLayout(spk::Layout *p_layout)
	{
		if (p_layout == nullptr)
		{
			return;
		}

		for (auto it = _elements.begin(); it != _elements.end(); ++it)
		{
			if (*it != nullptr && (*it)->layout() == p_layout)
			{
				_elements.erase(it);
				releaseSizeCache();
				return;
			}
		}

		throw std::runtime_error("Layout not contained by the layout");
	}

	void Layout::setElementPadding(const spk::Vector2UInt &p_padding)
	{
		_elementPadding = p_padding;
		releaseSizeCache();
	}

	const spk::Vector2UInt &Layout::elementPadding() const
	{
		return _elementPadding;
	}

	const std::vector<std::unique_ptr<Layout::Element>> &Layout::elements() const
	{
		return _elements;
	}

	std::vector<std::unique_ptr<Layout::Element>> &Layout::elements()
	{
		return _elements;
	}
}
