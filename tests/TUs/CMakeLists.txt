find_package(GTest REQUIRED)
find_package(Stb REQUIRED)

include(GoogleTest)

set(SPARKLE_TEST_SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/srcs)
set(SPARKLE_TEST_UTILS_DIR ${SPARKLE_TEST_SRC_DIR}/utils)

file(GLOB_RECURSE SPARKLE_TEST_UTILS_SOURCES
    CONFIGURE_DEPENDS
    ${SPARKLE_TEST_UTILS_DIR}/*.cpp
)

include_directories(
	${CMAKE_CURRENT_SOURCE_DIR}/includes
)

file(GLOB_RECURSE SPARKLE_TEST_SOURCES
    CONFIGURE_DEPENDS
    ${SPARKLE_TEST_SRC_DIR}/*.cpp
)
add_executable(SparkleTest ${SPARKLE_TEST_SOURCES})
if(ENABLE_COVERAGE)
    sparkle_enable_coverage(SparkleTest)
endif()

target_link_libraries(SparkleTest PRIVATE
	Sparkle
	GTest::GTest
)
target_include_directories(SparkleTest PRIVATE ${Stb_INCLUDE_DIR})

gtest_discover_tests(SparkleTest PROPERTIES LABELS all)

if(ENABLE_COVERAGE)
    add_custom_target(coverage
        COMMAND
            ${CMAKE_COMMAND}
                -DLLVM_COV_EXECUTABLE=${LLVM_COV_EXECUTABLE}
                -DLLVM_PROFDATA_EXECUTABLE=${LLVM_PROFDATA_EXECUTABLE}
                -DBINARY_DIR=${CMAKE_BINARY_DIR}
                -DSOURCE_DIR=${PROJECT_SOURCE_DIR}
                -DTEST_EXECUTABLE=$<TARGET_FILE:SparkleTest>
                -P ${PROJECT_SOURCE_DIR}/cmake/run_llvm_coverage.cmake
        DEPENDS SparkleTest
        WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
        COMMENT "Running Sparkle tests and generating LLVM coverage report"
        VERBATIM
    )
endif()
