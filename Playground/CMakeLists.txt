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

target_compile_features(SparklePlayground PRIVATE cxx_std_20)
