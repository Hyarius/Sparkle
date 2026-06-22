#include "structures/system/win32/spk_winapi_cursor.hpp"

#include <mutex>
#include <stdexcept>
#include <unordered_map>
#include <utility>

#include "structures/system/win32/spk_winapi_window.hpp"
#include "utils/spk_winapi_helpers.hpp"

namespace
{
	std::mutex &cursorRegistryMutex()
	{
		static std::mutex instance;
		return instance;
	}

	std::unordered_map<std::string, HCURSOR> &cursorRegistry()
	{
		static std::unordered_map<std::string, HCURSOR> instance = []() {
			std::unordered_map<std::string, HCURSOR> m;
			m.emplace("Arrow", LoadCursorW(nullptr, MAKEINTRESOURCEW(32512)));
			m.emplace("IBeam", LoadCursorW(nullptr, MAKEINTRESOURCEW(32513)));
			m.emplace("Hand", LoadCursorW(nullptr, MAKEINTRESOURCEW(32649)));
			m.emplace("Cross", LoadCursorW(nullptr, MAKEINTRESOURCEW(32515)));
			m.emplace("Wait", LoadCursorW(nullptr, MAKEINTRESOURCEW(32514)));
			m.emplace("ResizeNS", LoadCursorW(nullptr, MAKEINTRESOURCEW(32645)));
			m.emplace("ResizeWE", LoadCursorW(nullptr, MAKEINTRESOURCEW(32644)));
			m.emplace("ResizeNWSE", LoadCursorW(nullptr, MAKEINTRESOURCEW(32642)));
			m.emplace("ResizeNESW", LoadCursorW(nullptr, MAKEINTRESOURCEW(32643)));
			m.emplace("ResizeAll", LoadCursorW(nullptr, MAKEINTRESOURCEW(32646)));
			return m;
		}();
		return instance;
	}
}

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

	Cursor::Cursor(Cursor &&p_other) noexcept :
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

	Cursor &Cursor::operator=(Cursor &&p_other) noexcept
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

	Cursor Cursor::resizeNS()
	{
		return Cursor(LoadCursorW(nullptr, MAKEINTRESOURCEW(32645)));
	}

	Cursor Cursor::resizeWE()
	{
		return Cursor(LoadCursorW(nullptr, MAKEINTRESOURCEW(32644)));
	}

	Cursor Cursor::resizeNWSE()
	{
		return Cursor(LoadCursorW(nullptr, MAKEINTRESOURCEW(32642)));
	}

	Cursor Cursor::resizeNESW()
	{
		return Cursor(LoadCursorW(nullptr, MAKEINTRESOURCEW(32643)));
	}

	Cursor Cursor::resizeAll()
	{
		return Cursor(LoadCursorW(nullptr, MAKEINTRESOURCEW(32646)));
	}

	void Cursor::registerCursor(const std::string &p_name, const Cursor &p_cursor)
	{
		HCURSOR copy = CopyCursor(p_cursor.handle());
		if (copy == nullptr)
		{
			throw std::runtime_error("spk::Cursor::registerCursor: CopyCursor failed for '" + p_name + "'");
		}

		std::scoped_lock lock(cursorRegistryMutex());
		auto &registry = cursorRegistry();
		auto it = registry.find(p_name);
		if (it != registry.end())
		{
			DestroyCursor(it->second);
			it->second = copy;
		}
		else
		{
			registry.emplace(p_name, copy);
		}
	}

	Cursor Cursor::get(const std::string &p_name)
	{
		std::scoped_lock lock(cursorRegistryMutex());
		auto it = cursorRegistry().find(p_name);
		if (it == cursorRegistry().end())
		{
			throw std::invalid_argument("spk::Cursor::get: unknown cursor name '" + p_name + "'");
		}
		return Cursor(it->second, false);
	}

	void Cursor::activate() const
	{
		if (_handle == nullptr)
		{
			throw std::runtime_error("spk::Cursor::activate called on an empty cursor");
		}

		SetCursor(_handle);
	}

	void Cursor::activate(const spk::WindowRuntime &p_window) const
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
