# Definition of the Sparkle static library target.

add_library(Sparkle STATIC)
add_library(Sparkle::Sparkle ALIAS Sparkle)

target_sources(Sparkle
    PRIVATE
        ${SPARKLE_SOURCES}
        ${SPARKLE_HEADERS}
        "${SPARKLE_RESOURCES_HEADER}"
)

# Make sure the embedded resource header is generated before Sparkle compiles.
add_dependencies(Sparkle SparkleResourcesHeaderTarget)

target_compile_features(Sparkle PUBLIC cxx_std_23)

set_target_properties(Sparkle
    PROPERTIES
        CXX_EXTENSIONS OFF
)

target_include_directories(Sparkle
    PUBLIC
        $<BUILD_INTERFACE:${SPARKLE_INCLUDE_DIR}>
        $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
    PRIVATE
        ${Stb_INCLUDE_DIR}
)

target_link_libraries(Sparkle
    PUBLIC
        GLEW::GLEW
        OpenGL::GL
)

if(WIN32)
    target_compile_definitions(Sparkle PUBLIC NOMINMAX)

    target_link_libraries(Sparkle
        PUBLIC
            dinput8
            dxguid
            ws2_32
    )
endif()
