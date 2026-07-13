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

target_compile_features(PlaygroundCore PUBLIC cxx_std_23)

# Resources are resolved at runtime by pg::resourceRoot(): the --resource-path CLI
# override when given (the development workflow — point it at this source tree), else a
# "resources" folder next to the executable. Nothing is copied at build time; the test
# binary pins its lookup to the source tree instead (see tests/CMakeLists.txt).

if(SPARKLE_BUILD_PLAYGROUND OR SPARKLE_BUILD_TESTS)
	add_executable(SparklePlayground "${CMAKE_CURRENT_SOURCE_DIR}/srcs/main.cpp")

	target_link_libraries(SparklePlayground
		PRIVATE
			PlaygroundCore
	)
endif()

if(SPARKLE_BUILD_TESTS)
	add_subdirectory(tests)
endif()
