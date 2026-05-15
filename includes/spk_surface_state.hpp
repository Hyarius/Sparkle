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

		void invalidate();
		[[nodiscard]] bool isValid() const;
	};
}
