# Build-time options for Sparkle. All options are project-prefixed so they do
# not collide with consumer projects that add Sparkle via add_subdirectory().

option(SPARKLE_BUILD_TESTS "Build Sparkle unit tests" OFF)
option(SPARKLE_BUILD_PLAYGROUND "Build the Sparkle playground executable" OFF)
option(SPARKLE_ENABLE_CLANG_TIDY "Enable clang-tidy analysis during builds" OFF)
option(SPARKLE_ENABLE_COVERAGE "Enable code coverage instrumentation" OFF)
option(SPARKLE_INSTALL "Enable Sparkle install/package rules" ON)

set(SPARKLE_COVERAGE_MIN_LINE_RATE
    "95"
    CACHE STRING
    "Minimum line coverage percentage required by the coverage target."
)

# Coverage drives the unit-test binary, so it cannot run without the tests.
if(SPARKLE_ENABLE_COVERAGE AND NOT SPARKLE_BUILD_TESTS)
    message(FATAL_ERROR "SPARKLE_ENABLE_COVERAGE=ON requires SPARKLE_BUILD_TESTS=ON.")
endif()
