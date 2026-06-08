#include <gtest/gtest.h>

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
	return RUN_ALL_TESTS();
}
