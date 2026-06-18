#include "structures/system/win32/spk_winapi_class.hpp"

#include <utility>

#include "utils/spk_winapi_helpers.hpp"

namespace spk
{
	WindowClass::WindowClass(std::string p_name, WNDPROC p_windowProcedure, HINSTANCE p_instance) :
		_instance(p_instance),
		_name(std::move(p_name)),
		_nativeName(toWideString(_name))
	{
		if (_instance == nullptr)
		{
			throw std::invalid_argument("spk::WinAPI::Class requires a valid module instance");
		}
		if (_name.empty() == true)
		{
			throw std::invalid_argument("spk::WinAPI::Class requires a non-empty class name");
		}
		if (p_windowProcedure == nullptr)
		{
			throw std::invalid_argument("spk::WinAPI::Class requires a valid window procedure");
		}

		WNDCLASSEXW descriptor{};
		descriptor.cbSize = sizeof(descriptor);
		descriptor.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
		descriptor.lpfnWndProc = p_windowProcedure;
		descriptor.hInstance = _instance;
		descriptor.hCursor = LoadCursorW(nullptr, MAKEINTRESOURCEW(32512));
		descriptor.hbrBackground = nullptr;
		descriptor.lpszClassName = _nativeName.c_str();

		if (RegisterClassExW(&descriptor) == 0)
		{
			const DWORD errorCode = GetLastError();
			if (errorCode != ERROR_CLASS_ALREADY_EXISTS)
			{
				throwLastError("RegisterClassExW");
			}
			_isRegistered = false;
			return;
		}

		_isRegistered = true;
	}

	WindowClass::WindowClass(WindowClass&& p_other) noexcept :
		_instance(p_other._instance),
		_name(std::move(p_other._name)),
		_nativeName(std::move(p_other._nativeName)),
		_isRegistered(p_other._isRegistered)
	{
		p_other._instance = nullptr;
		p_other._isRegistered = false;
	}

	WindowClass::~WindowClass()
	{
		if (_isRegistered == true && _instance != nullptr && _nativeName.empty() == false)
		{
			UnregisterClassW(_nativeName.c_str(), _instance);
		}
	}

	WindowClass& WindowClass::operator=(WindowClass&& p_other) noexcept
	{
		if (this == &p_other)
		{
			return *this;
		}

		if (_isRegistered == true && _instance != nullptr && _nativeName.empty() == false)
		{
			UnregisterClassW(_nativeName.c_str(), _instance);
		}

		_instance = p_other._instance;
		_name = std::move(p_other._name);
		_nativeName = std::move(p_other._nativeName);
		_isRegistered = p_other._isRegistered;
		p_other._instance = nullptr;
		p_other._isRegistered = false;
		return *this;
	}

	HINSTANCE WindowClass::instance() const
	{
		return _instance;
	}

	const std::string& WindowClass::name() const
	{
		return _name;
	}
}
