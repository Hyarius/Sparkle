#include "structures/system/device/window/spk_surface_state.hpp"

namespace spk
{
	void SurfaceState::invalidate()
	{
		_isValid.store(false);
	}

	bool SurfaceState::isValid() const
	{
		return _isValid.load();
	}

	void SurfaceState::setRect(const spk::Rect2D& p_rect)
	{
		_rect = p_rect;
	}

	const spk::Rect2D& SurfaceState::rect() const
	{
		return _rect;
	}
}
