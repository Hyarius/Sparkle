#include "spk_activable_trait.hpp"

#include <utility>

namespace spk
{
	void ActivableTrait::flushPending()
	{
		if (_pendingActivationState.has_value() == false)
		{
			return;
		}

		const bool shouldActivate = *_pendingActivationState;
		_pendingActivationState.reset();

		if (shouldActivate == true)
		{
			activate();
		}
		else
		{
			deactivate();
		}
	}

	bool ActivableTrait::isActivated() const
	{
		return _isActivated;
	}

	void ActivableTrait::activate()
	{
		if (_isActivated == true)
		{
			return;
		}

		if (isBlocked() == true)
		{
			if (isDelayBlocked() == true)
			{
				_pendingActivationState = true;
				setPending();
			}

			return;
		}

		_isActivated = true;
		_activationProvider.trigger();
	}

	void ActivableTrait::deactivate()
	{
		if (_isActivated == false)
		{
			return;
		}

		if (isBlocked() == true)
		{
			if (isDelayBlocked() == true)
			{
				_pendingActivationState = false;
				setPending();
			}

			return;
		}

		_isActivated = false;
		_deactivationProvider.trigger();
	}

	ActivableTrait::Contract ActivableTrait::subscribeToActivation(Callback p_callback)
	{
		return _activationProvider.subscribe(std::move(p_callback));
	}

	ActivableTrait::Contract ActivableTrait::subscribeToDeactivation(Callback p_callback)
	{
		return _deactivationProvider.subscribe(std::move(p_callback));
	}
}