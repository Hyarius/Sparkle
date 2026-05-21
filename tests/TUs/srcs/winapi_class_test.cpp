#include <gtest/gtest.h>

#ifdef _WIN32

#include <atomic>
#include <stdexcept>
#include <string>

#include <Windows.h>

#include "winapi/spk_winapi_class.hpp"

namespace
{
	std::atomic_uint g_uniqueId = 0;

	[[nodiscard]] std::string uniqueClassName(const std::string& p_prefix)
	{
		return p_prefix + "." + std::to_string(GetCurrentProcessId()) + "." + std::to_string(++g_uniqueId);
	}

	LRESULT CALLBACK testWindowProcedure(HWND p_handle, UINT p_message, WPARAM p_wParam, LPARAM p_lParam)
	{
		return DefWindowProcW(p_handle, p_message, p_wParam, p_lParam);
	}
}

TEST(WinAPIClassTest, ConstructorRejectsInvalidInputs)
{
	EXPECT_THROW(spk::WinAPI::Class("InvalidInstance", &testWindowProcedure, nullptr), std::invalid_argument);
	EXPECT_THROW(spk::WinAPI::Class("", &testWindowProcedure), std::invalid_argument);
	EXPECT_THROW(spk::WinAPI::Class("InvalidProcedure", nullptr), std::invalid_argument);
}

TEST(WinAPIClassTest, RegistersDuplicateClassAndMoveTransfersOwnership)
{
	const std::string className = uniqueClassName("Sparkle.Test.Class");

	spk::WinAPI::Class registeredClass(className, &testWindowProcedure);
	EXPECT_EQ(registeredClass.name(), className);
	EXPECT_EQ(registeredClass.instance(), GetModuleHandleW(nullptr));

	spk::WinAPI::Class duplicateClass(className, &testWindowProcedure);
	EXPECT_EQ(duplicateClass.name(), className);

	spk::WinAPI::Class movedClass(std::move(registeredClass));
	EXPECT_EQ(movedClass.name(), className);
	EXPECT_EQ(movedClass.instance(), GetModuleHandleW(nullptr));
	EXPECT_EQ(registeredClass.instance(), nullptr);

	spk::WinAPI::Class assignedClass(uniqueClassName("Sparkle.Test.Assigned"), &testWindowProcedure);
	assignedClass = std::move(movedClass);
	EXPECT_EQ(assignedClass.name(), className);
	EXPECT_EQ(assignedClass.instance(), GetModuleHandleW(nullptr));
	EXPECT_EQ(movedClass.instance(), nullptr);
}

TEST(WinAPIClassTest, SelfMoveAssignmentPreservesRegisteredClass)
{
	const std::string className = uniqueClassName("Sparkle.Test.Class.SelfMove");

	spk::WinAPI::Class windowClass(className, &testWindowProcedure);
	windowClass = std::move(windowClass);

	EXPECT_EQ(windowClass.name(), className);
	EXPECT_EQ(windowClass.instance(), GetModuleHandleW(nullptr));
}

#endif
