find_package(GTest REQUIRED)

include(GoogleTest)

set(SPARKLE_TEST_SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/srcs)
set(SPARKLE_TEST_UTILS_DIR ${SPARKLE_TEST_SRC_DIR}/test_utils)

file(GLOB_RECURSE SPARKLE_TEST_UTILS_SOURCES
    CONFIGURE_DEPENDS
    ${SPARKLE_TEST_UTILS_DIR}/*.cpp
)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/includes)

file(GLOB_RECURSE SPARKLE_TEST_SOURCES
    CONFIGURE_DEPENDS
    ${SPARKLE_TEST_SRC_DIR}/*.cpp
)
add_executable(SparkleTest ${SPARKLE_TEST_SOURCES})
target_link_libraries(SparkleTest PRIVATE
	Sparkle
	GTest::GTest
	GTest::Main
)

gtest_discover_tests(SparkleTest PROPERTIES LABELS all)
