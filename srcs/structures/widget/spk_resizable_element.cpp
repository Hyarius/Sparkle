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
		// Install a constant generator (not a one-shot cached value) so the size survives
		// releaseSizeCache(): a value set with set() would be dropped on the next release
		// and silently revert to the default generator.
		_minimalSize.configure([p_minimalValue]() { return p_minimalValue; });
	}

	void ResizableElement::setFixedSize(const spk::Vector2UInt &p_fixedValue)
	{
		_fixedSize.configure([p_fixedValue]() { return p_fixedValue; });
	}

	void ResizableElement::setMaximalSize(const spk::Vector2UInt &p_maximalValue)
	{
		_maximalSize.configure([p_maximalValue]() { return p_maximalValue; });
	}

	spk::Vector2UInt ResizableElement::minimalSize() const
	{
		return _minimalSize.get();
	}

	spk::Vector2UInt ResizableElement::preferredSizeFor(const spk::Vector2UInt &p_availableSize) const
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
