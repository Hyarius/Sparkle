#include "spk_surface_state.hpp"

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
}
