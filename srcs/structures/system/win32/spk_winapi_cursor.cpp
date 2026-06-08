#include "structures/system/win32/spk_winapi_cursor.hpp"

#ifdef _WIN32

#include <stdexcept>
#include <utility>

#include "utils/spk_winapi_helpers.hpp"
#include "structures/system/win32/spk_winapi_window.hpp"

namespace spk
{
	Cursor::Cursor() = default;

	Cursor::Cursor(HCURSOR p_handle, bool p_takeOwnership) :
		_handle(p_handle),
		_ownsHandle(p_takeOwnership)
	{
		if (_handle == nullptr)
		{
			throw std::invalid_argument("spk::Cursor requires a valid cursor handle");
		}
	}

	Cursor::Cursor(Cursor&& p_other) noexcept :
		_handle(p_other._handle),
		_ownsHandle(p_other._ownsHandle)
	{
		p_other._handle = nullptr;
		p_other._ownsHandle = false;
	}

	Cursor::~Cursor()
	{
		if (_ownsHandle == true && _handle != nullptr)
		{
			DestroyCursor(_handle);
		}
	}

	Cursor& Cursor::operator=(Cursor&& p_other) noexcept
	{
		if (this == &p_other)
		{
			return *this;
		}

		if (_ownsHandle == true && _handle != nullptr)
		{
			DestroyCursor(_handle);
		}

		_handle = p_other._handle;
		_ownsHandle = p_other._ownsHandle;
		p_other._handle = nullptr;
		p_other._ownsHandle = false;
		return *this;
	}

	Cursor Cursor::arrow()
	{
		return Cursor(LoadCursorW(nullptr, MAKEINTRESOURCEW(32512)));
	}

	Cursor Cursor::ibeam()
	{
		return Cursor(LoadCursorW(nullptr, MAKEINTRESOURCEW(32513)));
	}

	Cursor Cursor::hand()
	{
		return Cursor(LoadCursorW(nullptr, MAKEINTRESOURCEW(32649)));
	}

	Cursor Cursor::cross()
	{
		return Cursor(LoadCursorW(nullptr, MAKEINTRESOURCEW(32515)));
	}

	Cursor Cursor::wait()
	{
		return Cursor(LoadCursorW(nullptr, MAKEINTRESOURCEW(32514)));
	}

	void Cursor::activate() const
	{
		if (_handle == nullptr)
		{
			throw std::runtime_error("spk::Cursor::activate called on an empty cursor");
		}

		SetCursor(_handle);
	}

	void Cursor::activate(const spk::WindowRuntime& p_window) const
	{
		p_window.setCursor(*this);
		activate();
	}

	HCURSOR Cursor::handle() const
	{
		return _handle;
	}

	bool Cursor::isValid() const
	{
		return _handle != nullptr;
	}
}

#endif
