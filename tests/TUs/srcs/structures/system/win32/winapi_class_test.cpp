#include <gtest/gtest.h>

#include <atomic>
#include <stdexcept>
#include <string>

#include <Windows.h>

#include "structures/system/win32/spk_winapi_class.hpp"

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
	EXPECT_THROW(spk::WindowClass("InvalidInstance", &testWindowProcedure, nullptr), std::invalid_argument);
	EXPECT_THROW(spk::WindowClass("", &testWindowProcedure), std::invalid_argument);
	EXPECT_THROW(spk::WindowClass("InvalidProcedure", nullptr), std::invalid_argument);
}

TEST(WinAPIClassTest, RegistersDuplicateClassAndMoveTransfersOwnership)
{
	const std::string className = uniqueClassName("Sparkle.Test.Class");

	spk::WindowClass registeredClass(className, &testWindowProcedure);
	EXPECT_EQ(registeredClass.name(), className);
	EXPECT_EQ(registeredClass.instance(), GetModuleHandleW(nullptr));

	spk::WindowClass duplicateClass(className, &testWindowProcedure);
	EXPECT_EQ(duplicateClass.name(), className);

	spk::WindowClass movedClass(std::move(registeredClass));
	EXPECT_EQ(movedClass.name(), className);
	EXPECT_EQ(movedClass.instance(), GetModuleHandleW(nullptr));
	EXPECT_EQ(registeredClass.instance(), nullptr);

	spk::WindowClass assignedClass(uniqueClassName("Sparkle.Test.Assigned"), &testWindowProcedure);
	assignedClass = std::move(movedClass);
	EXPECT_EQ(assignedClass.name(), className);
	EXPECT_EQ(assignedClass.instance(), GetModuleHandleW(nullptr));
	EXPECT_EQ(movedClass.instance(), nullptr);
}

TEST(WinAPIClassTest, SelfMoveAssignmentPreservesRegisteredClass)
{
	const std::string className = uniqueClassName("Sparkle.Test.Class.SelfMove");

	spk::WindowClass windowClass(className, &testWindowProcedure);
	windowClass = std::move(windowClass);

	EXPECT_EQ(windowClass.name(), className);
	EXPECT_EQ(windowClass.instance(), GetModuleHandleW(nullptr));
}
