cmake_minimum_required(VERSION 3.16)

project(SparklePlayground LANGUAGES CXX)

file(GLOB_RECURSE SPARKLE_PLAYGROUND_SOURCES
	CONFIGURE_DEPENDS
	"${CMAKE_CURRENT_SOURCE_DIR}/srcs/*.cpp"
)

add_executable(SparklePlayground ${SPARKLE_PLAYGROUND_SOURCES})

target_link_libraries(SparklePlayground
	PRIVATE
		Sparkle::Sparkle
)

target_include_directories(SparklePlayground
	PRIVATE
		"${CMAKE_CURRENT_SOURCE_DIR}/srcs"
)

# Absolute path to the Playground resources so the executable can load them
# regardless of the working directory it is launched from.
target_compile_definitions(SparklePlayground
	PRIVATE
		PG_RESOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}/resources"
)

target_compile_features(SparklePlayground PRIVATE cxx_std_23)
