# Source, header and resource collection for the Sparkle library.
#
# Resources are embedded into a single generated header (a global inline map
# keyed by relative path). The library and the tests both include that header,
# so it must hold every resource any translation unit looks up. Test fonts are
# therefore only appended when SPARKLE_BUILD_TESTS is ON; a plain library or
# install build embeds production resources only.

set(SPARKLE_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/srcs")
set(SPARKLE_INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/includes")
set(SPARKLE_RESOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/resources")

file(GLOB_RECURSE SPARKLE_SOURCES
    CONFIGURE_DEPENDS
    "${SPARKLE_SOURCE_DIR}/*.cpp"
)

file(GLOB_RECURSE SPARKLE_HEADERS
    CONFIGURE_DEPENDS
    "${SPARKLE_INCLUDE_DIR}/*.hpp"
    "${SPARKLE_INCLUDE_DIR}/*.h"
)

if(EXISTS "${SPARKLE_INCLUDE_DIR}/sparkle")
    list(APPEND SPARKLE_HEADERS "${SPARKLE_INCLUDE_DIR}/sparkle")
endif()

# Production resources embedded in the main library.
file(GLOB_RECURSE SPARKLE_RESOURCE_FILES
    CONFIGURE_DEPENDS
    LIST_DIRECTORIES false
    RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}"
    "${SPARKLE_RESOURCE_DIR}/*"
)

set(SPARKLE_EMBEDDED_RESOURCE_FILES ${SPARKLE_RESOURCE_FILES})

# Test-only resources share the single generated header, so they are added to
# the embedded set exclusively when the tests are part of the build.
if(SPARKLE_BUILD_TESTS)
    file(GLOB_RECURSE SPARKLE_TEST_RESOURCE_FILES
        CONFIGURE_DEPENDS
        LIST_DIRECTORIES false
        RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}"
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/TUs/resources/fonts/*"
    )
    list(APPEND SPARKLE_EMBEDDED_RESOURCE_FILES ${SPARKLE_TEST_RESOURCE_FILES})
endif()

# The resource converter (sparkleResourcesConverter) is a build-time tool that
# turns the resource files into the generated header consumed below.
include(cmake/add_resources.cmake)
add_subdirectory(tools)

add_resources(${SPARKLE_EMBEDDED_RESOURCE_FILES})
