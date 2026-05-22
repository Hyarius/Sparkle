#include "window/spk_surface_state.hpp"

namespace spk
{
	ISurfaceState::~ISurfaceState() = default;
	
	void ISurfaceState::invalidate()
	{
		_isValid.store(false);
	}

	bool ISurfaceState::isValid() const
	{
		return _isValid.load();
	}

	void ISurfaceState::setRect(const spk::Rect2D& p_rect)
	{
		_rect = p_rect;
	}

	const spk::Rect2D& ISurfaceState::rect() const
	{
		return _rect;
	}
}
