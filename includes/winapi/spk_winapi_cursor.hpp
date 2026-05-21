#pragma once

#ifdef _WIN32

#include <Windows.h>

namespace spk::WinAPI
{
	class Window;

	class Cursor
	{
	private:
		HCURSOR _handle = nullptr;
		bool _ownsHandle = false;

	public:
		Cursor();
		explicit Cursor(HCURSOR p_handle, bool p_takeOwnership = false);
		Cursor(const Cursor&) = delete;
		Cursor(Cursor&& p_other) noexcept;
		~Cursor();

		Cursor& operator=(const Cursor&) = delete;
		Cursor& operator=(Cursor&& p_other) noexcept;

		[[nodiscard]] static Cursor arrow();
		[[nodiscard]] static Cursor ibeam();
		[[nodiscard]] static Cursor hand();
		[[nodiscard]] static Cursor cross();
		[[nodiscard]] static Cursor wait();

		void activate() const;
		void activate(const spk::WinAPI::Window& p_window) const;

		[[nodiscard]] HCURSOR handle() const;
		[[nodiscard]] bool isValid() const;
	};
}

#endif
