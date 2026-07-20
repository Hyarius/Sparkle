#include "structures/design_pattern/spk_activable_trait.hpp"

#include <utility>

namespace spk
{
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

		_executeOrBlock([this]() noexcept {
			try
			{
				if (_isActivated == false)
				{
					_isActivated = true;
					_activationProvider.trigger();
				}
			} catch (...)
			{
				std::terminate();
			}
		});
	}

	void ActivableTrait::deactivate()
	{
		if (_isActivated == false)
		{
			return;
		}

		_executeOrBlock([this]() noexcept {
			try
			{
				if (_isActivated == true)
				{
					_isActivated = false;
					_deactivationProvider.trigger();
				}
			} catch (...)
			{
				std::terminate();
			}
		});
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
