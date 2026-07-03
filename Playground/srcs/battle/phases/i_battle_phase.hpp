#pragma once

#include <string_view>
namespace pg
{
	class IBattlePhase
	{
	public:
		virtual ~IBattlePhase() = default;
		virtual void enter() = 0;
		virtual void tick(float) = 0;
		virtual void exit() = 0;
		[[nodiscard]] virtual std::string_view name() const noexcept = 0;
	};
}
