# CPack configuration: produce a distributable ZIP archive of Sparkle.

set(CPACK_PACKAGE_NAME "Sparkle")
set(CPACK_PACKAGE_VENDOR "EreliaStudio")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "A C++23 OpenGL rendering library.")
set(CPACK_PACKAGE_VERSION ${PROJECT_VERSION})
set(CPACK_RESOURCE_FILE_README "${CMAKE_CURRENT_SOURCE_DIR}/README.md")
set(CPACK_GENERATOR "ZIP")

include(CPack)
