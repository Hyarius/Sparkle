cmake_minimum_required(VERSION 3.16)

project(SparkleUIShowcase VERSION 0.0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

option(
    SPARKLE_UI_SHOWCASE_USE_LOCAL_SPARKLE
    "Build Sparkle from a local source checkout instead of using find_package(Sparkle)."
    ON
)

set(
    SPARKLE_UI_SHOWCASE_SPARKLE_SOURCE_DIR
    "${CMAKE_CURRENT_LIST_DIR}/.."
    CACHE PATH
    "Path to the Sparkle repository root when SPARKLE_UI_SHOWCASE_USE_LOCAL_SPARKLE is ON."
)

if (SPARKLE_UI_SHOWCASE_USE_LOCAL_SPARKLE)
    if (NOT EXISTS "${SPARKLE_UI_SHOWCASE_SPARKLE_SOURCE_DIR}/CMakeLists.txt")
        message(FATAL_ERROR "SPARKLE_UI_SHOWCASE_SPARKLE_SOURCE_DIR does not point to a CMake project.")
    endif()

    add_subdirectory(
        "${SPARKLE_UI_SHOWCASE_SPARKLE_SOURCE_DIR}"
        "${CMAKE_CURRENT_BINARY_DIR}/sparkle-build"
        EXCLUDE_FROM_ALL
    )
else()
    find_package(Sparkle CONFIG REQUIRED)
endif()

file(GLOB_RECURSE SPARKLE_UI_SHOWCASE_HEADERS
    CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/includes/*.hpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/includes/*.h"
)

file(GLOB_RECURSE SPARKLE_UI_SHOWCASE_SOURCES
    CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/srcs/*.cpp"
)

add_executable(SparkleUIShowcase
    ${SPARKLE_UI_SHOWCASE_HEADERS}
    ${SPARKLE_UI_SHOWCASE_SOURCES}
)

target_include_directories(SparkleUIShowcase
    PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/includes"
)

target_link_libraries(SparkleUIShowcase
    PRIVATE
        Sparkle::Sparkle
)

target_compile_features(SparkleUIShowcase PRIVATE cxx_std_23)
