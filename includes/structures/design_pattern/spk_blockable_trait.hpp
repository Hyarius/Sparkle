#pragma once

#include <cstddef>
#include <functional>
#include <memory>

namespace spk
{
	class BlockableTrait
	{
	public:
		enum class Mode
		{
			Ignore,
			Delay
		};

	private:
		struct State
		{
			size_t nbIgnoreBlocks = 0;
			size_t nbDelayBlocks = 0;
			std::function<void()> delayedOperation = nullptr;
		};

		std::shared_ptr<State> _state = std::make_shared<State>();

	protected:
		BlockableTrait();
		virtual ~BlockableTrait();

		void deferUntilUnblocked(std::function<void()> p_operation);

	public:
		class Blocker
		{
		private:
			std::weak_ptr<State> _state;
			Mode _mode = Mode::Ignore;

			void _tryRunDelayedOperation(State &p_state);

		public:
			Blocker() = default;
			explicit Blocker(const std::shared_ptr<State> &p_state, Mode p_mode);
			~Blocker();

			Blocker(const Blocker &) = delete;
			Blocker &operator=(const Blocker &) = delete;

			Blocker(Blocker &&p_other) noexcept;
			Blocker &operator=(Blocker &&p_other) noexcept;

			void release();
			bool isValid() const;
		};

		Blocker block(Mode p_mode = Mode::Ignore);

		bool isBlocked() const;
		bool isIgnoreBlocked() const;
		bool isDelayBlocked() const;
	};
}
