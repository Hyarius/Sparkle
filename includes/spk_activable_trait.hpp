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

		void flushPending() override;

	public:
		ActivableTrait(const ActivableTrait&) = delete;
		ActivableTrait& operator=(const ActivableTrait&) = delete;

		ActivableTrait(ActivableTrait&&) noexcept = delete;
		ActivableTrait& operator=(ActivableTrait&&) noexcept = delete;

		bool isActivated() const;
		void activate();

		void deactivate();

		Contract subscribeToActivation(Callback p_callback);
		Contract subscribeToDeactivation(Callback p_callback);
	};
}