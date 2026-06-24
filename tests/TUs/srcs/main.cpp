#include <gtest/gtest.h>

#include <cstdlib>
#include <cstdio>

#include "structures/graphics/opengl/opengl_wrapper_test_utils.hpp"

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
	// GoogleTest environments are already torn down; avoid post-main GL/CRT static teardown crashes.
	std::_Exit(result);
}
