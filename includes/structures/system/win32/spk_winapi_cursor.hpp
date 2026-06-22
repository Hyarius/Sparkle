#pragma once

#include <string>

#include <Windows.h>

namespace spk
{
	class WindowRuntime;

	class Cursor
	{
	private:
		HCURSOR _handle = nullptr;
		bool _ownsHandle = false;

	public:
		Cursor();
		explicit Cursor(HCURSOR p_handle, bool p_takeOwnership = false);
		Cursor(const Cursor &) = delete;
		Cursor(Cursor &&p_other) noexcept;
		~Cursor();

		Cursor &operator=(const Cursor &) = delete;
		Cursor &operator=(Cursor &&p_other) noexcept;

		[[nodiscard]] static Cursor arrow();
		[[nodiscard]] static Cursor ibeam();
		[[nodiscard]] static Cursor hand();
		[[nodiscard]] static Cursor cross();
		[[nodiscard]] static Cursor wait();
		[[nodiscard]] static Cursor resizeNS();
		[[nodiscard]] static Cursor resizeWE();
		[[nodiscard]] static Cursor resizeNWSE();
		[[nodiscard]] static Cursor resizeNESW();
		[[nodiscard]] static Cursor resizeAll();

		static void registerCursor(const std::string &p_name, const Cursor &p_cursor);
		[[nodiscard]] static Cursor get(const std::string &p_name);

		void activate() const;
		void activate(const spk::WindowRuntime &p_window) const;

		[[nodiscard]] HCURSOR handle() const;
		[[nodiscard]] bool isValid() const;
	};
}
