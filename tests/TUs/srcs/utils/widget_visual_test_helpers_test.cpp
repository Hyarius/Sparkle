#include <filesystem>
#include <memory>
#include <vector>

#include <gtest/gtest.h>
#include <stb_image_write.h>

#include "spk_render_unit_builder.hpp"
#include "spk_widget_visual_test_helpers.hpp"

#if defined(_WIN32) && defined(SPARKLE_GPU_BACKEND_OPENGL)
#include <Windows.h>
#include <GL/gl.h>
#endif

#if defined(_WIN32) && defined(SPARKLE_GPU_BACKEND_OPENGL)
namespace
{
	class ClearColorCommand : public spk::RenderCommand
	{
	private:
		float _red = 0.0f;
		float _green = 0.0f;
		float _blue = 0.0f;
		float _alpha = 1.0f;

	public:
		ClearColorCommand(float p_red, float p_green, float p_blue, float p_alpha) :
			_red(p_red),
			_green(p_green),
			_blue(p_blue),
			_alpha(p_alpha)
		{
		}

		void execute(spk::IRenderContext& p_renderContext) override
		{
			(void)p_renderContext;
			glClearColor(_red, _green, _blue, _alpha);
			glClear(GL_COLOR_BUFFER_BIT);
		}
	};

	class SolidColorWidget : public spk::Widget
	{
	private:
		float _red = 0.0f;
		float _green = 0.0f;
		float _blue = 0.0f;
		float _alpha = 1.0f;

	protected:
		spk::RenderUnit _buildRenderUnit() const override
		{
			spk::RenderUnitBuilder builder;
			builder.emplace<ClearColorCommand>(_red, _green, _blue, _alpha);
			return builder.build();
		}

	public:
		SolidColorWidget(float p_red, float p_green, float p_blue, float p_alpha) :
			spk::Widget("SolidColorWidget"),
			_red(p_red),
			_green(p_green),
			_blue(p_blue),
			_alpha(p_alpha)
		{
			activate();
		}
	};
}

TEST(WidgetVisualTestHelpers, CompareSnapshotSupportsWidgetStyleVisualTests)
{
	const spk::Rect2D captureRect(0, 0, 8, 8);
	const std::filesystem::path expectedDir = spk::test::expectedDirectory() / "SolidColorWidget";
	std::filesystem::create_directories(expectedDir);

	const std::filesystem::path expectedPath = expectedDir / "green.png";
	std::vector<unsigned char> expectedPixels(static_cast<std::size_t>(captureRect.width()) * static_cast<std::size_t>(captureRect.height()) * 4, 0);
	for (std::size_t index = 0; index < expectedPixels.size(); index += 4)
	{
		expectedPixels[index + 0] = 0;
		expectedPixels[index + 1] = 255;
		expectedPixels[index + 2] = 0;
		expectedPixels[index + 3] = 255;
	}

	ASSERT_NE(
		stbi_write_png(
			expectedPath.string().c_str(),
			static_cast<int>(captureRect.width()),
			static_cast<int>(captureRect.height()),
			4,
			expectedPixels.data(),
			static_cast<int>(captureRect.width()) * 4),
		0);

	SolidColorWidget widget(0.0f, 1.0f, 0.0f, 1.0f);
	const sparkle_test::ImageComparisonResult result = spk::test::compareSnapshot(widget, "SolidColorWidget", "green", captureRect);

	EXPECT_TRUE(result.matches);
	EXPECT_EQ(result.differentPixelCount, 0);
}
#endif
