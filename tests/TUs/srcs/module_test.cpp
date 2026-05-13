#include <gtest/gtest.h>

#include "spk_module.hpp"
#include "window_test_utils.hpp"

namespace
{
	class TestModule : public spk::IModule
	{
	};
}

TEST(IModuleTest, BindStoresWidgetPointerAndAllowsNull)
{
	TestModule module;
	sparkle_test::RecordingWidget widget("Widget");

	module.bind(&widget);
	EXPECT_EQ(module.widget(), &widget);

	module.bind(nullptr);
	EXPECT_EQ(module.widget(), nullptr);
}
