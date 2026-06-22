# Optional clang-tidy integration.
#
# When SPARKLE_ENABLE_CLANG_TIDY is ON this provides three ways to lint:
#   * the CXX_CLANG_TIDY target property, so a normal build of Sparkle runs
#     clang-tidy inline (parallelised by the build tool; used by CI);
#   * the "sparkle-clang-tidy" target, an on-demand check over all sources;
#   * the "sparkle-clang-tidy-fix" target, the same run with fixes applied.
#
# The property is set on the Sparkle target only (not the global
# CMAKE_CXX_CLANG_TIDY, which would also touch unrelated targets).

if(NOT SPARKLE_ENABLE_CLANG_TIDY)
    return()
endif()

find_program(SPARKLE_CLANG_TIDY_EXECUTABLE NAMES clang-tidy)

if(NOT SPARKLE_CLANG_TIDY_EXECUTABLE)
    message(FATAL_ERROR
        "SPARKLE_ENABLE_CLANG_TIDY is ON but clang-tidy was not found. "
        "Install LLVM or disable the option."
    )
endif()

# Keep clang-tidy aligned with the exceptions-enabled build (some Windows
# toolchains default to -fno-exceptions when invoked in clang-cl mode).
set(SPARKLE_CLANG_TIDY_COMMON_ARGS --warnings-as-errors=*)

if(MSVC)
    list(APPEND SPARKLE_CLANG_TIDY_COMMON_ARGS --extra-arg=/EHsc)
else()
    list(APPEND SPARKLE_CLANG_TIDY_COMMON_ARGS
        --extra-arg=-fexceptions
        --extra-arg=-fcxx-exceptions
    )
endif()

set_target_properties(Sparkle
    PROPERTIES
        CXX_CLANG_TIDY "${SPARKLE_CLANG_TIDY_EXECUTABLE};${SPARKLE_CLANG_TIDY_COMMON_ARGS}"
)

# Arguments shared by the standalone check and fix targets. They read the
# configured compilation database and inspect headers as well as sources.
set(SPARKLE_CLANG_TIDY_RUN_ARGS
    ${SPARKLE_CLANG_TIDY_COMMON_ARGS}
    -header-filter=.*
    -p "${CMAKE_BINARY_DIR}"
)

# Check only: report diagnostics and fail on warnings, without touching files.
add_custom_target(sparkle-clang-tidy
    COMMAND ${SPARKLE_CLANG_TIDY_EXECUTABLE}
            ${SPARKLE_CLANG_TIDY_RUN_ARGS}
            ${SPARKLE_SOURCES}
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    COMMENT "Running clang-tidy across Sparkle sources"
    VERBATIM
)

# Fix: same analysis, but apply the suggested fixes in place.
add_custom_target(sparkle-clang-tidy-fix
    COMMAND ${SPARKLE_CLANG_TIDY_EXECUTABLE}
            ${SPARKLE_CLANG_TIDY_RUN_ARGS}
            --fix
            --fix-errors
            --format-style=file
            ${SPARKLE_SOURCES}
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    COMMENT "Applying clang-tidy fixes across Sparkle sources"
    VERBATIM
)

# The sources include the generated resource header, so it must exist before
# clang-tidy parses them. This lets either target run on a clean tree without
# a prior full build.
add_dependencies(sparkle-clang-tidy SparkleResourcesHeaderTarget)
add_dependencies(sparkle-clang-tidy-fix SparkleResourcesHeaderTarget)
