# Third-party dependencies for the Sparkle library.
#
# GLEW and OpenGL are part of the public, installed interface (see
# SparkleConfig.cmake.in). Stb is only used internally by the implementation,
# so it stays a private dependency and is not re-exported to consumers.

find_package(Stb REQUIRED)
find_package(GLEW REQUIRED)
find_package(OpenGL REQUIRED)
