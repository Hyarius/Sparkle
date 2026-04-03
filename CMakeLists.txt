cmake_minimum_required(VERSION 3.16)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

project(Sparkle VERSION 0.0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_OBJECT_PATH_MAX 4096)

if(WIN32)
    add_compile_definitions(NOMINMAX)
endif()

include(CMakePackageConfigHelpers)
include(GNUInstallDirs)
include(cmake/add_resources.cmake)

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

add_resources(${SPARKLE_RESOURCE_FILES})

find_package(GLEW REQUIRED)
find_package(OpenGL REQUIRED)

add_library(Sparkle STATIC ${SPARKLE_SOURCES} ${SPARKLE_HEADERS})
add_library(Sparkle::Sparkle ALIAS Sparkle)

add_dependencies(Sparkle SparkleResourcesHeaderTarget)
target_sources(Sparkle PRIVATE "${SPARKLE_RESOURCES_HEADER}")

target_compile_features(Sparkle PUBLIC cxx_std_20)

target_include_directories(Sparkle
    PUBLIC
        $<BUILD_INTERFACE:${SPARKLE_INCLUDE_DIR}>
        $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

target_link_libraries(Sparkle
    PUBLIC
        GLEW::GLEW
        OpenGL::GL
)

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

install(
    FILES
        "${CMAKE_CURRENT_BINARY_DIR}/SparkleConfig.cmake"
        "${CMAKE_CURRENT_BINARY_DIR}/SparkleConfigVersion.cmake"
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
endif()