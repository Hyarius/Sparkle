#include <gtest/gtest.h>

#include <memory>
#include <vector>

#include "structures/graphics/opengl/opengl_wrapper_test_utils.hpp"
#include "structures/graphics/rendering/snapshot/spk_render_snapshot.hpp"
#include "structures/graphics/rendering/unit/spk_render_unit.hpp"
#include "structures/graphics/rendering/unit/spk_render_unit_builder.hpp"

namespace
{
	class CountingRenderCommand : public spk::RenderCommand
	{
	private:
		int &_executionCount;

	public:
		explicit CountingRenderCommand(int &p_executionCount) :
			_executionCount(p_executionCount)
		{
		}

		void execute(spk::RenderContext &) override
		{
			++_executionCount;
		}
	};
}

TEST(RenderUnitTest, ExecuteSkipsNullCommandsAndExecutesValidCommands)
{
	sparkle_test::OpenGLTestContext context;
	int executionCount = 0;

	std::vector<std::unique_ptr<spk::RenderCommand>> commands;
	commands.push_back(nullptr);
	commands.push_back(std::make_unique<CountingRenderCommand>(executionCount));

	spk::RenderUnit unit(std::move(commands));

	unit.execute(context.renderContext());

	EXPECT_EQ(executionCount, 1);
}

TEST(RenderUnitBuilderTest, AddTransfersAnExistingCommand)
{
	sparkle_test::OpenGLTestContext context;
	int executionCount = 0;
	spk::RenderUnitBuilder builder;

	builder.add(std::make_unique<CountingRenderCommand>(executionCount));

	EXPECT_EQ(builder.size(), 1u);
	builder.build().execute(context.renderContext());
	EXPECT_EQ(executionCount, 1);
}

TEST(RenderSnapshotTest, ExecuteSkipsNullUnitsAndExecutesValidUnits)
{
	sparkle_test::OpenGLTestContext context;
	int executionCount = 0;

	std::vector<std::unique_ptr<spk::RenderCommand>> commands;
	commands.push_back(std::make_unique<CountingRenderCommand>(executionCount));

	std::vector<std::shared_ptr<spk::RenderUnit>> units;
	units.push_back(nullptr);
	units.push_back(std::make_shared<spk::RenderUnit>(std::move(commands)));

	spk::RenderSnapshot snapshot(std::move(units));

	snapshot.execute(context.renderContext());

	EXPECT_EQ(executionCount, 1);
}
