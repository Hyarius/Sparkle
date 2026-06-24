#pragma once

#include <functional>

#include "structures/design_pattern/spk_contract_provider.hpp"

namespace spk
{
	// Gives a type an integer priority and notifies subscribers when it changes.
	// Convention: HIGHER priority is processed FIRST.
	class PriorizableTrait
	{
	public:
		using Priority = int;
		using Callback = std::function<void()>;
		using Contract = spk::ContractProvider<>::Contract;

	private:
		Priority _priority = 0;
		spk::ContractProvider<> _priorityChangeProvider;

	protected:
		PriorizableTrait() = default;
		explicit PriorizableTrait(Priority p_priority) :
			_priority(p_priority)
		{
		}

		virtual ~PriorizableTrait() = default;

	public:
		PriorizableTrait(const PriorizableTrait &) = delete;
		PriorizableTrait &operator=(const PriorizableTrait &) = delete;

		PriorizableTrait(PriorizableTrait &&) noexcept = delete;
		PriorizableTrait &operator=(PriorizableTrait &&) noexcept = delete;

		[[nodiscard]] Priority priority() const
		{
			return _priority;
		}

		void setPriority(Priority p_priority)
		{
			if (_priority == p_priority)
			{
				return;
			}

			_priority = p_priority;
			_priorityChangeProvider.trigger();
		}

		[[nodiscard]] Contract subscribeToPriorityChange(Callback p_callback)
		{
			return _priorityChangeProvider.subscribe(std::move(p_callback));
		}
	};
}
