#include <gtest/gtest.h>

int main(int p_argc, char **p_argv)
{
	::testing::InitGoogleTest(&p_argc, p_argv);
	return RUN_ALL_TESTS();
}
