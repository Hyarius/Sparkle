#include <gtest/gtest.h>

#include <cstddef>
#include <limits>
#include <stdexcept>
#include <type_traits>

#include "structures/graphics/rendering/command/spk_directional_light_update_render_command.hpp"
#include "structures/graphics/spk_directional_light.hpp"

TEST(DirectionalLight, MatchesTheDocumentedStd140Block)
{
	EXPECT_EQ(sizeof(spk::DirectionalLight), 48u);
	EXPECT_EQ(alignof(spk::DirectionalLight), 16u);
	EXPECT_TRUE(std::is_trivially_copyable_v<spk::DirectionalLight>);
	EXPECT_EQ(offsetof(spk::DirectionalLight, direction), 0u);
	EXPECT_EQ(offsetof(spk::DirectionalLight, color), 16u);
	EXPECT_EQ(offsetof(spk::DirectionalLight, ambient), 32u);
}

TEST(DirectionalLightUpdateRenderCommand, AcceptsTheLightBlockAndValidatesBindingRange)
{
	const spk::DirectionalLight light{
		.direction = {0.0f, -1.0f, 0.0f},
		.color = {1.0f, 0.9f, 0.8f, 1.0f},
		.ambient = 0.25f};
	EXPECT_NO_THROW(spk::DirectionalLightUpdateRenderCommand(3, light));
	if constexpr (sizeof(std::size_t) > sizeof(GLuint))
	{
		EXPECT_THROW(
			spk::DirectionalLightUpdateRenderCommand(
				static_cast<std::size_t>(std::numeric_limits<GLuint>::max()) + 1u,
				light),
			std::out_of_range);
	}
}
