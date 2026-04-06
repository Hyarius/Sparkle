#pragma once

#include <algorithm>
#include <functional>
#include <memory>
#include <optional>
#include <tuple>
#include <utility>
#include <vector>

#include "spk_blockable_trait.hpp"

namespace spk
{
	template <typename... TArguments>
	class ContractProvider : public BlockableTrait
	{
	public:
		using Callback = std::function<void(TArguments...)>;
		using Blocker = BlockableTrait::Blocker;

	private:
		using StoredArguments = std::tuple<std::decay_t<TArguments>...>;

	private:
		struct Link : public BlockableTrait
		{
			Callback function = nullptr;

		protected:
			void flushPending() override
			{
			}
		};

		class TriggerGuard
		{
		private:
			bool& _flag;

		public:
			explicit TriggerGuard(bool& p_flag) :
				_flag(p_flag)
			{
				_flag = true;
			}

			~TriggerGuard()
			{
				_flag = false;
			}

			TriggerGuard(const TriggerGuard&) = delete;
			TriggerGuard& operator=(const TriggerGuard&) = delete;

			TriggerGuard(TriggerGuard&&) = delete;
			TriggerGuard& operator=(TriggerGuard&&) = delete;
		};

	public:
		class Contract
		{
			friend class ContractProvider;

		public:
			using Blocker = BlockableTrait::Blocker;

		private:
			std::shared_ptr<Link> _link = nullptr;

			explicit Contract(const std::shared_ptr<Link>& p_link) :
				_link(p_link)
			{
			}

			bool isBlocked() const
			{
				return (_link != nullptr && _link->isBlocked());
			}

		public:
			Contract() = default;

			~Contract()
			{
				resign();
			}

			Contract(const Contract&) = delete;
			Contract& operator=(const Contract&) = delete;

			Contract(Contract&& p_other) noexcept :
				_link(std::move(p_other._link))
			{
			}

			Contract& operator=(Contract&& p_other) noexcept
			{
				if (this != &p_other)
				{
					resign();
					_link = std::move(p_other._link);
				}

				return *this;
			}

			void resign()
			{
				if (_link != nullptr)
				{
					_link->function = nullptr;
					_link = nullptr;
				}
			}

			void relinquish()
			{
				_link = nullptr;
			}

			bool isValid() const
			{
				return (_link != nullptr && _link->function != nullptr);
			}

			Blocker block(Mode p_mode = Mode::Ignore)
			{
				if (_link == nullptr)
				{
					return Blocker();
				}

				return _link->block(p_mode);
			}
		};

	private:
		std::vector<std::shared_ptr<Link>> _links;
		bool _isTriggering = false;
		std::optional<StoredArguments> _lastTriggerArguments;

		void cleanup()
		{
			if (_isTriggering)
			{
				return;
			}

			_links.erase(
				std::remove_if(
					_links.begin(),
					_links.end(),
					[](const std::shared_ptr<Link>& p_link)
					{
						return (p_link == nullptr || p_link->function == nullptr);
					}),
				_links.end());
		}

	protected:
		void flushPending() override
		{
			if (_lastTriggerArguments.has_value() == false)
			{
				return;
			}

			StoredArguments arguments = std::move(*_lastTriggerArguments);
			_lastTriggerArguments.reset();

			std::apply(
				[this](auto&&... p_arguments)
				{
					trigger(p_arguments...);
				},
				arguments);
		}

	public:
		ContractProvider() = default;
		~ContractProvider() override = default;

		ContractProvider(const ContractProvider&) = delete;
		ContractProvider& operator=(const ContractProvider&) = delete;

		ContractProvider(ContractProvider&&) noexcept = delete;
		ContractProvider& operator=(ContractProvider&&) noexcept = delete;

		Contract subscribe(Callback p_callback)
		{
			std::shared_ptr<Link> newLink = std::make_shared<Link>();
			newLink->function = std::move(p_callback);

			_links.push_back(newLink);

			return Contract(newLink);
		}

		void trigger(TArguments... p_arguments)
		{
			if (isBlocked())
			{
				_lastTriggerArguments.emplace(p_arguments...);
				setPending();
				return;
			}

			if (_isTriggering)
			{
				return;
			}

			{
				TriggerGuard guard(_isTriggering);

				const size_t initialCount = _links.size();

				for (size_t i = 0; i < initialCount; ++i)
				{
					std::shared_ptr<Link> element = _links[i];

					if (element == nullptr ||
						element->function == nullptr ||
						element->isBlocked())
					{
						continue;
					}

					element->function(p_arguments...);
				}
			}

			cleanup();
		}

		void invalidateAllContracts()
		{
			for (auto& element : _links)
			{
				if (element != nullptr)
				{
					element->function = nullptr;
				}
			}

			cleanup();
		}

		size_t nbContracts() const
		{
			size_t result = 0;

			for (const auto& element : _links)
			{
				if (element != nullptr && element->function != nullptr)
				{
					++result;
				}
			}

			return result;
		}
	};
}