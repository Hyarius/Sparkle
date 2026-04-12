#pragma once

#include <atomic>

namespace spk
{
	class ISurfaceState
	{
	private:
		std::atomic<bool> _isValid = true;

	public:
		virtual ~ISurfaceState();

		virtual void invalidate();
		[[nodiscard]] virtual bool isValid() const;
	};
}
