cmake_minimum_required(VERSION 3.16)

project(SparklePlayground LANGUAGES CXX)

find_package(nlohmann_json CONFIG REQUIRED)
find_package(GTest CONFIG REQUIRED)

file(GLOB_RECURSE PLAYGROUND_CORE_SOURCES
	CONFIGURE_DEPENDS
	"${CMAKE_CURRENT_SOURCE_DIR}/srcs/*.cpp"
)
list(FILTER PLAYGROUND_CORE_SOURCES EXCLUDE REGEX "/main\\.cpp$")

add_library(PlaygroundCore STATIC ${PLAYGROUND_CORE_SOURCES})

target_link_libraries(PlaygroundCore
	PUBLIC
		Sparkle::Sparkle
		nlohmann_json::nlohmann_json
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

file(GLOB_RECURSE ERELIA_TOOLS_CORE_SOURCES
	CONFIGURE_DEPENDS
	"${CMAKE_CURRENT_SOURCE_DIR}/tools/*.cpp"
)
list(FILTER ERELIA_TOOLS_CORE_SOURCES EXCLUDE REGEX "/main\\.cpp$")

add_library(EreliaToolsCore STATIC ${ERELIA_TOOLS_CORE_SOURCES})
target_link_libraries(EreliaToolsCore PUBLIC PlaygroundCore)
target_include_directories(EreliaToolsCore PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")
target_compile_features(EreliaToolsCore PUBLIC cxx_std_23)

add_executable(EreliaTools "${CMAKE_CURRENT_SOURCE_DIR}/tools/main.cpp")
target_link_libraries(EreliaTools PRIVATE EreliaToolsCore)

file(GLOB_RECURSE PLAYGROUND_TEST_SOURCES
	CONFIGURE_DEPENDS
	"${CMAKE_CURRENT_SOURCE_DIR}/tests/*.cpp"
)

add_executable(PlaygroundTests ${PLAYGROUND_TEST_SOURCES})

target_link_libraries(PlaygroundTests
	PRIVATE
		PlaygroundCore
		EreliaToolsCore
		GTest::gtest_main
)

target_include_directories(PlaygroundTests
	PRIVATE
		"${CMAKE_CURRENT_SOURCE_DIR}/tests"
)

include(GoogleTest)
gtest_discover_tests(PlaygroundTests PROPERTIES LABELS playground)
