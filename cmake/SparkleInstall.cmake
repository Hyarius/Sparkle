# Install and CMake package-config rules for Sparkle.
#
# Produces the canonical package files so that find_package(Sparkle REQUIRED)
# works against an installed tree:
#   - SparkleConfig.cmake
#   - SparkleConfigVersion.cmake
#   - SparkleTargets.cmake

set(SPARKLE_INSTALL_CONFIGDIR "${CMAKE_INSTALL_LIBDIR}/cmake/Sparkle")

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
    DESTINATION ${SPARKLE_INSTALL_CONFIGDIR}
)

write_basic_package_version_file(
    "${CMAKE_CURRENT_BINARY_DIR}/SparkleConfigVersion.cmake"
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY SameMajorVersion
)

configure_package_config_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake/SparkleConfig.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/SparkleConfig.cmake"
    INSTALL_DESTINATION ${SPARKLE_INSTALL_CONFIGDIR}
)

install(
    FILES
        "${CMAKE_CURRENT_BINARY_DIR}/SparkleConfig.cmake"
        "${CMAKE_CURRENT_BINARY_DIR}/SparkleConfigVersion.cmake"
    DESTINATION ${SPARKLE_INSTALL_CONFIGDIR}
)

# Export the build tree so it can be consumed without installing.
export(
    EXPORT SparkleTargets
    FILE "${CMAKE_CURRENT_BINARY_DIR}/SparkleTargets.cmake"
    NAMESPACE Sparkle::
)
