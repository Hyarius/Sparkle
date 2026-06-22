cmake_minimum_required(VERSION 3.16)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

project(Sparkle VERSION 0.0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_OBJECT_PATH_MAX 4096)

if(WIN32)
    add_compile_definitions(NOMINMAX)
endif()

include(CMakePackageConfigHelpers)
include(GNUInstallDirs)
include(cmake/add_resources.cmake)
include(cmake/add_coverage.cmake)

if(CMAKE_SIZEOF_VOID_P EQUAL 8)
    set(ARCH_FOLDER x64)
else()
    set(ARCH_FOLDER x86)
endif()

set(CONFIGURATION_KEY "$<IF:$<CONFIG:Debug>,Debug,$<CONFIG>>")

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/out/bin/${CONFIGURATION_KEY}/${ARCH_FOLDER}")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/out/lib/${CONFIGURATION_KEY}/${ARCH_FOLDER}")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/out/lib/${CONFIGURATION_KEY}/${ARCH_FOLDER}")

set(SPARKLE_SRC_DIR "${CMAKE_CURRENT_SOURCE_DIR}/srcs")
set(SPARKLE_INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/includes")
set(SPARKLE_RESOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/resources")

option(ENABLE_PLAYGROUND "Build the Sparkle playground executable" OFF)

add_subdirectory(tools)

file(GLOB_RECURSE SPARKLE_SOURCES
    CONFIGURE_DEPENDS
    "${SPARKLE_SRC_DIR}/*.cpp"
)

file(GLOB_RECURSE SPARKLE_HEADERS
    CONFIGURE_DEPENDS
    "${SPARKLE_INCLUDE_DIR}/*.hpp"
    "${SPARKLE_INCLUDE_DIR}/*.h"
)

if(EXISTS "${SPARKLE_INCLUDE_DIR}/sparkle")
    list(APPEND SPARKLE_HEADERS "${SPARKLE_INCLUDE_DIR}/sparkle")
endif()

file(GLOB_RECURSE SPARKLE_RESOURCE_FILES
    CONFIGURE_DEPENDS
    LIST_DIRECTORIES false
    RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}"
    "${SPARKLE_RESOURCE_DIR}/*"
)

file(GLOB_RECURSE SPARKLE_TEST_RESOURCE_FILES
    CONFIGURE_DEPENDS
    LIST_DIRECTORIES false
    RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}"
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/TUs/resources/fonts/*"
)

add_resources(${SPARKLE_RESOURCE_FILES} ${SPARKLE_TEST_RESOURCE_FILES})

    ########################################
    #          Static analysis             #
    ########################################
option(SPARKLE_ENABLE_CLANG_TIDY "Enable clang-tidy analysis during builds" OFF)

if (SPARKLE_ENABLE_CLANG_TIDY)
    find_program(SPARKLE_CLANG_TIDY_EXECUTABLE NAMES clang-tidy)

    if (SPARKLE_CLANG_TIDY_EXECUTABLE)
        # Keep clang-tidy aligned with the exceptions-enabled build (some Windows toolchains
        # default to -fno-exceptions when invoked in clang-cl mode).
        set(SPARKLE_CLANG_TIDY_COMMON_ARGS --warnings-as-errors=*)

        if (MSVC)
            list(APPEND SPARKLE_CLANG_TIDY_COMMON_ARGS --extra-arg=/EHsc)
        else()
            list(APPEND SPARKLE_CLANG_TIDY_COMMON_ARGS --extra-arg=-fexceptions --extra-arg=-fcxx-exceptions)
        endif()

        set(SPARKLE_CLANG_TIDY_FIX_ARGS ${SPARKLE_CLANG_TIDY_COMMON_ARGS} --fix --format-style=file)
        set(CMAKE_CXX_CLANG_TIDY "${SPARKLE_CLANG_TIDY_EXECUTABLE};${SPARKLE_CLANG_TIDY_COMMON_ARGS}")
    else()
        message(FATAL_ERROR "SPARKLE_ENABLE_CLANG_TIDY is ON but clang-tidy was not found. Install LLVM or disable the option.")
    endif()

    if (SPARKLE_CLANG_TIDY_EXECUTABLE)
        add_custom_target(clang-tidy-fix
            COMMAND ${SPARKLE_CLANG_TIDY_EXECUTABLE} ${SPARKLE_CLANG_TIDY_FIX_ARGS} -p ${CMAKE_BINARY_DIR} ${SPARKLE_SOURCES}
            WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
            COMMENT "Applying clang-tidy fixes across Sparkle sources"
            VERBATIM
        )
    endif()
endif()

    ########################################
    #          Coverage instrumentation    #
    ########################################
option(SPARKLE_ENABLE_COVERAGE "Enable code coverage instrumentation for supported toolchains" OFF)
set(SPARKLE_COVERAGE_MIN_LINE_RATE "95" CACHE STRING "Minimum line coverage percentage required by the coverage target.")

if (SPARKLE_ENABLE_COVERAGE AND NOT ENABLE_TESTS)
    message(FATAL_ERROR "SPARKLE_ENABLE_COVERAGE=ON requires ENABLE_TESTS=ON so that coverage can run the unit suite.")
endif()

find_package(Stb REQUIRED)

find_package(GLEW REQUIRED)
find_package(OpenGL REQUIRED)

add_library(Sparkle STATIC ${SPARKLE_SOURCES} ${SPARKLE_HEADERS})
add_library(Sparkle::Sparkle ALIAS Sparkle)

add_dependencies(Sparkle SparkleResourcesHeaderTarget)
target_sources(Sparkle PRIVATE "${SPARKLE_RESOURCES_HEADER}")

target_compile_features(Sparkle PUBLIC cxx_std_23)

target_include_directories(Sparkle
    PUBLIC
        $<BUILD_INTERFACE:${SPARKLE_INCLUDE_DIR}>
        $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
    PRIVATE
        ${Stb_INCLUDE_DIR}
)

target_link_libraries(Sparkle PUBLIC GLEW::GLEW OpenGL::GL)

if(WIN32)
    target_link_libraries(Sparkle
        PUBLIC
            dinput8
            dxguid
            ws2_32
    )
endif()

set(INSTALL_CONFIGDIR "${CMAKE_INSTALL_LIBDIR}/cmake/Sparkle")

install(
    TARGETS Sparkle
    EXPORT SparkleTargets
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

install(
    DIRECTORY "${SPARKLE_INCLUDE_DIR}/"
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

install(
    FILES "${SPARKLE_RESOURCES_HEADER}"
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

install(
    EXPORT SparkleTargets
    FILE SparkleTargets.cmake
    NAMESPACE Sparkle::
    DESTINATION ${INSTALL_CONFIGDIR}
)

write_basic_package_version_file(
    "${CMAKE_CURRENT_BINARY_DIR}/SparkleConfigVersion.cmake"
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY SameMajorVersion
)

configure_package_config_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake/SparkleConfig.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/SparkleConfig.cmake"
    INSTALL_DESTINATION ${INSTALL_CONFIGDIR}
)

write_basic_package_version_file(
    "${CMAKE_CURRENT_BINARY_DIR}/sparkle-config-version.cmake"
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY SameMajorVersion
)

configure_package_config_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake/SparkleConfig.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/sparkle-config.cmake"
    INSTALL_DESTINATION ${INSTALL_CONFIGDIR}
)

install(
    FILES
        "${CMAKE_CURRENT_BINARY_DIR}/SparkleConfig.cmake"
        "${CMAKE_CURRENT_BINARY_DIR}/SparkleConfigVersion.cmake"
        "${CMAKE_CURRENT_BINARY_DIR}/sparkle-config.cmake"
        "${CMAKE_CURRENT_BINARY_DIR}/sparkle-config-version.cmake"
    DESTINATION ${INSTALL_CONFIGDIR}
)

export(
    EXPORT SparkleTargets
    FILE "${CMAKE_CURRENT_BINARY_DIR}/SparkleTargets.cmake"
    NAMESPACE Sparkle::
)

if (ENABLE_TESTS)
	enable_testing()

	add_subdirectory(tests)

	if (SPARKLE_ENABLE_COVERAGE)
		sparkle_enable_coverage(
			TEST_TARGET SparkleTest
			LIBRARIES Sparkle
		)
	endif()
endif()

if (ENABLE_PLAYGROUND)
	add_subdirectory(Playground)
endif()

set(CPACK_PACKAGE_NAME "Sparkle")
set(CPACK_PACKAGE_VENDOR "EreliaStudio")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "A C++23 OpenGL rendering library.")
set(CPACK_PACKAGE_VERSION ${PROJECT_VERSION})
set(CPACK_RESOURCE_FILE_README "${CMAKE_CURRENT_SOURCE_DIR}/README.md")
set(CPACK_GENERATOR "ZIP")
include(CPack)
