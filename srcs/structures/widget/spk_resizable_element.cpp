#include "structures/widget/spk_resizable_element.hpp"

#include <utility>

namespace spk
{
	void ResizableElement::configureSizeGenerators(
		SizeGenerator p_minimalGenerator,
		SizeGenerator p_fixedGenerator,
		SizeGenerator p_maximalGenerator)
	{
		configureMinimalSizeGenerator(std::move(p_minimalGenerator));
		configureFixedSizeGenerator(std::move(p_fixedGenerator));
		configureMaximalSizeGenerator(std::move(p_maximalGenerator));
	}

	void ResizableElement::configureMinimalSizeGenerator(SizeGenerator p_generator)
	{
		_minimalSize.configure(std::move(p_generator));
	}

	void ResizableElement::configureFixedSizeGenerator(SizeGenerator p_generator)
	{
		_fixedSize.configure(std::move(p_generator));
	}

	void ResizableElement::configureMaximalSizeGenerator(SizeGenerator p_generator)
	{
		_maximalSize.configure(std::move(p_generator));
	}

	void ResizableElement::releaseSizeCache() const
	{
		releaseMinimalSize();
		releaseFixedSize();
		releaseMaximalSize();
	}

	void ResizableElement::releaseMinimalSize() const
	{
		_minimalSize.release();
	}

	void ResizableElement::releaseFixedSize() const
	{
		_fixedSize.release();
	}

	void ResizableElement::releaseMaximalSize() const
	{
		_maximalSize.release();
	}

	void ResizableElement::setMinimalSize(const spk::Vector2UInt &p_minimalValue)
	{
		_minimalSize.set(p_minimalValue);
	}

	void ResizableElement::setFixedSize(const spk::Vector2UInt &p_fixedValue)
	{
		_fixedSize.set(p_fixedValue);
	}

	void ResizableElement::setMaximalSize(const spk::Vector2UInt &p_maximalValue)
	{
		_maximalSize.set(p_maximalValue);
	}

	spk::Vector2UInt ResizableElement::minimalSize() const
	{
		return _minimalSize.get();
	}

	spk::Vector2UInt ResizableElement::minimalSizeFor(const spk::Vector2UInt &p_availableSize) const
	{
		(void)p_availableSize;
		return minimalSize();
	}

	spk::Vector2UInt ResizableElement::fixedSize() const
	{
		return _fixedSize.get();
	}

	spk::Vector2UInt ResizableElement::maximalSize() const
	{
		return _maximalSize.get();
	}
}
