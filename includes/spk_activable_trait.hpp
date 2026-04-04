#pragma once

#include <functional>
#include <optional>
#include <utility>

#include "spk_blockable_trait.hpp"
#include "spk_contract_provider.hpp"

namespace spk
{
	class ActivableTrait : public BlockableTrait
	{
	public:
		using Callback = std::function<void()>;
		using Contract = ContractProvider<>::Contract;
		using Blocker = BlockableTrait::Blocker;

	private:
		bool _isActivated = false;
		std::optional<bool> _pendingActivationState;

		ContractProvider<> _activationProvider;
		ContractProvider<> _deactivationProvider;

	protected:
		ActivableTrait() = default;
		~ActivableTrait() = default;

		void flushPending() override
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

	public:
		ActivableTrait(const ActivableTrait&) = delete;
		ActivableTrait& operator=(const ActivableTrait&) = delete;

		ActivableTrait(ActivableTrait&&) noexcept = delete;
		ActivableTrait& operator=(ActivableTrait&&) noexcept = delete;

		bool isActivated() const
		{
			return _isActivated;
		}

		void activate()
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

		void deactivate()
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

		Contract subscribeToActivation(Callback p_callback)
		{
			return _activationProvider.subscribe(std::move(p_callback));
		}

		Contract subscribeToDeactivation(Callback p_callback)
		{
			return _deactivationProvider.subscribe(std::move(p_callback));
		}
	};
}