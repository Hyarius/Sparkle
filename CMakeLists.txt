cmake_minimum_required(VERSION 3.20)

project(Sparkle
    VERSION 0.0.1
    LANGUAGES CXX
)

# Build-wide tooling settings (not target specific).
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_OBJECT_PATH_MAX 4096)

include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

include(cmake/SparkleOptions.cmake)
include(cmake/SparkleSources.cmake)
include(cmake/SparkleDependencies.cmake)
include(cmake/SparkleTarget.cmake)
include(cmake/SparkleClangTidy.cmake)

if(SPARKLE_BUILD_TESTS OR SPARKLE_BUILD_PLAYGROUND)
    enable_testing()
endif()

if(SPARKLE_BUILD_TESTS)
    add_subdirectory(tests)

    if(SPARKLE_ENABLE_COVERAGE)
        include(cmake/add_coverage.cmake)
        sparkle_enable_coverage(
            TEST_TARGET SparkleTest
            LIBRARIES Sparkle
        )
    endif()
endif()

if(SPARKLE_BUILD_PLAYGROUND OR SPARKLE_BUILD_TESTS)
    add_subdirectory(Playground)
endif()

if(SPARKLE_INSTALL)
    include(cmake/SparkleInstall.cmake)
    include(cmake/SparklePackage.cmake)
endif()
