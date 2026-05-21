#pragma once

#include <cstddef>
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
			BlockableTrait* owner = nullptr;
			size_t nbIgnoreBlocks = 0;
			size_t nbDelayBlocks = 0;
			bool pending = false;
			bool alive = true;
		};

		std::shared_ptr<State> _state = std::make_shared<State>();

	protected:
		BlockableTrait();
		virtual ~BlockableTrait();

		virtual void flushPending() = 0;

		void setPending();

	public:
		class Blocker
		{
		private:
			std::weak_ptr<State> _state;
			Mode _mode = Mode::Ignore;

			void _tryFlushDelayed(State& p_state);

		public:
			Blocker() = default;
			explicit Blocker(const std::shared_ptr<State>& p_state, Mode p_mode);
			~Blocker();

			Blocker(const Blocker&) = delete;
			Blocker& operator=(const Blocker&) = delete;

			Blocker(Blocker&& p_other) noexcept;
			Blocker& operator=(Blocker&& p_other) noexcept;

			void release();
			bool isValid() const;
		};

		Blocker block(Mode p_mode = Mode::Ignore);

		bool isBlocked() const;
		bool isIgnoreBlocked() const;
		bool isDelayBlocked() const;
	};
}