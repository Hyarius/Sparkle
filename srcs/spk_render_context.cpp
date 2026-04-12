#include "spk_render_context.hpp"

#include <stdexcept>

namespace spk
{
	IRenderContext::IRenderContext(std::shared_ptr<ISurfaceState> p_surfaceState) :
		_surfaceState(std::move(p_surfaceState))
	{
		if (_surfaceState == nullptr)
		{
			throw std::invalid_argument("IRenderContext requires a valid surface state");
		}
	}

	IRenderContext::~IRenderContext() = default;

	std::shared_ptr<ISurfaceState> IRenderContext::surfaceState() const
	{
		return _surfaceState;
	}

	void IRenderContext::invalidate()
	{
		if (_surfaceState != nullptr)
		{
			_surfaceState->invalidate();
		}
	}

	bool IRenderContext::isValid() const
	{
		return (_surfaceState != nullptr && _surfaceState->isValid());
	}
}
