cmake_minimum_required(VERSION 3.16)

project(SparklePlayground LANGUAGES CXX)

file(GLOB_RECURSE PLAYGROUND_CORE_SOURCES
	CONFIGURE_DEPENDS
	"${CMAKE_CURRENT_SOURCE_DIR}/srcs/*.cpp"
)
list(FILTER PLAYGROUND_CORE_SOURCES EXCLUDE REGEX "/main\\.cpp$")

add_library(PlaygroundCore STATIC ${PLAYGROUND_CORE_SOURCES})

target_link_libraries(PlaygroundCore
	PUBLIC
		Sparkle::Sparkle
)

target_include_directories(PlaygroundCore
	PUBLIC
		"${CMAKE_CURRENT_SOURCE_DIR}/srcs"
)

# Absolute path to the Playground resources so every consumer can load them
# regardless of the working directory it is launched from.
target_compile_definitions(PlaygroundCore
	PUBLIC
		PG_RESOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}/resources"
)

target_compile_features(PlaygroundCore PUBLIC cxx_std_23)

add_executable(SparklePlayground "${CMAKE_CURRENT_SOURCE_DIR}/srcs/main.cpp")

target_link_libraries(SparklePlayground
	PRIVATE
		PlaygroundCore
)
