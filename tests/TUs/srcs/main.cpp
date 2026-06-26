#include <gtest/gtest.h>

#include <cstdlib>
#include <cstdio>

#include "structures/graphics/opengl/opengl_wrapper_test_utils.hpp"

#if defined(SPARKLE_ENABLE_LLVM_COVERAGE)
extern "C" int __llvm_profile_write_file(void);

namespace
{
	bool hasLLVMProfileFile()
	{
#if defined(_WIN32)
		char *value = nullptr;
		std::size_t valueSize = 0;
		if (_dupenv_s(&value, &valueSize, "LLVM_PROFILE_FILE") != 0)
		{
			return false;
		}
		std::free(value);
		return valueSize > 0;
#else
		return std::getenv("LLVM_PROFILE_FILE") != nullptr;
#endif
	}
}
#endif

class OpenGLGlobalEnvironment : public ::testing::Environment
{
public:
	void TearDown() override
	{
		sparkle_test::OpenGLTestContext::tearDownGlobal();
	}
};

int main(int argc, char** argv)
{
	::testing::InitGoogleTest(&argc, argv);
	::testing::AddGlobalTestEnvironment(new OpenGLGlobalEnvironment());
	const int result = RUN_ALL_TESTS();
	std::fflush(nullptr);
#if defined(SPARKLE_ENABLE_LLVM_COVERAGE)
	if (hasLLVMProfileFile() && __llvm_profile_write_file() != 0)
	{
		std::fprintf(stderr, "Failed to write LLVM profile data.\n");
		std::fflush(stderr);
	}
#endif
	std::_Exit(result);
}
