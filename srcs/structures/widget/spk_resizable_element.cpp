#include "structures/widget/spk_resizable_element.hpp"

#include <utility>

namespace spk
{
	ResizableElement::SizeHint::SizeHint(Generator p_minimalGenerator, Generator p_desiredGenerator, Generator p_maximalGenerator)
	{
		configure(std::move(p_minimalGenerator), std::move(p_desiredGenerator), std::move(p_maximalGenerator));
	}

	void ResizableElement::SizeHint::configure(Generator p_minimalGenerator, Generator p_desiredGenerator, Generator p_maximalGenerator)
	{
		configureMinimalGenerator(std::move(p_minimalGenerator));
		configureDesiredGenerator(std::move(p_desiredGenerator));
		configureMaximalGenerator(std::move(p_maximalGenerator));
	}

	void ResizableElement::SizeHint::configureMinimalGenerator(Generator p_generator)
	{
		_minimal.configure(std::move(p_generator));
	}

	void ResizableElement::SizeHint::configureDesiredGenerator(Generator p_generator)
	{
		_desired.configure(std::move(p_generator));
	}

	void ResizableElement::SizeHint::configureMaximalGenerator(Generator p_generator)
	{
		_maximal.configure(std::move(p_generator));
	}

	void ResizableElement::SizeHint::release() const
	{
		releaseMinimal();
		releaseDesired();
		releaseMaximal();
	}

	void ResizableElement::SizeHint::releaseMinimal() const
	{
		_minimal.release();
	}

	void ResizableElement::SizeHint::releaseDesired() const
	{
		_desired.release();
	}

	void ResizableElement::SizeHint::releaseMaximal() const
	{
		_maximal.release();
	}

	void ResizableElement::SizeHint::setMinimal(const spk::Vector2UInt& p_minimalValue)
	{
		_minimal.configure([p_minimalValue]() { return p_minimalValue; });
	}

	void ResizableElement::SizeHint::setDesired(const spk::Vector2UInt& p_desiredValue)
	{
		_desired.configure([p_desiredValue]() { return p_desiredValue; });
	}

	void ResizableElement::SizeHint::setMaximal(const spk::Vector2UInt& p_maximalValue)
	{
		_maximal.configure([p_maximalValue]() { return p_maximalValue; });
	}

	const spk::Vector2UInt& ResizableElement::SizeHint::minimal() const
	{
		return _minimal.get();
	}

	const spk::Vector2UInt& ResizableElement::SizeHint::desired() const
	{
		return _desired.get();
	}

	const spk::Vector2UInt& ResizableElement::SizeHint::maximal() const
	{
		return _maximal.get();
	}

	ResizableElement::SizeHint& ResizableElement::sizeHint()
	{
		return _sizeHint;
	}

	const ResizableElement::SizeHint& ResizableElement::sizeHint() const
	{
		return _sizeHint;
	}
}
