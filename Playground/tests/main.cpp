#include <gtest/gtest.h>

#include "core/paths.hpp"

#ifndef PG_TEST_RESOURCE_DIR
#	error "PG_TEST_RESOURCE_DIR must point at the source resources folder"
#endif

int main(int p_argc, char **p_argv)
{
	::testing::InitGoogleTest(&p_argc, p_argv);
	// Tests always run on a machine that has the checkout: point the resource lookup at
	// the live source tree, so data edits are visible without any deploy or sync step.
	pg::setResourceRootOverride(PG_TEST_RESOURCE_DIR);
	return RUN_ALL_TESTS();
}
