#pragma once

#include <cstddef>
#include <deque>
#include <functional>
#include <limits>
#include <memory>
#include <mutex>
#include <stdexcept>

namespace spk
{
	class BlockableTrait
	{
	public:
		enum class Mode
		{
			Ignore,
			Defer
		};

	protected:
		using Operation = std::move_only_function<void() noexcept>;

	private:
		struct State
		{
			mutable std::mutex mutex;
			std::size_t nbIgnoreBlocks = 0;
			std::size_t nbDeferBlocks = 0;
			std::deque<Operation> deferredOperations;
		};
		std::shared_ptr<State> _state = std::make_shared<State>();

	protected:
		BlockableTrait() = default;
		virtual ~BlockableTrait() = default;
		BlockableTrait(const BlockableTrait &) = delete;
		BlockableTrait &operator=(const BlockableTrait &) = delete;
		BlockableTrait(BlockableTrait &&) = delete;
		BlockableTrait &operator=(BlockableTrait &&) = delete;

		void _executeOrBlock(Operation p_operation);

	public:
		class Blocker
		{
			friend class BlockableTrait;

		private:
			std::weak_ptr<State> _state;
			Mode _mode = Mode::Ignore;
			explicit Blocker(const std::shared_ptr<State> &p_state, Mode p_mode);
			void _release(bool p_throwIfInvalid);

		public:
			Blocker() = default;
			~Blocker() noexcept;
			Blocker(const Blocker &) = delete;
			Blocker &operator=(const Blocker &) = delete;
			Blocker(Blocker &&p_other) noexcept;
			Blocker &operator=(Blocker &&p_other) noexcept;
			void release();
			[[nodiscard]] bool isValid() const noexcept;
		};

		[[nodiscard("The returned Blocker must remain alive")]] Blocker block(Mode p_mode = Mode::Ignore);
		[[nodiscard]] bool isBlocked() const noexcept;
		[[nodiscard]] bool isIgnoreBlocked() const noexcept;
		[[nodiscard]] bool isDeferBlocked() const noexcept;
	};
}
