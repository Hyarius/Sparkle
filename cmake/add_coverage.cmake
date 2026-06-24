include_guard(GLOBAL)

function(sparkle_enable_coverage)
    set(options)
    set(oneValueArgs TEST_TARGET)
    set(multiValueArgs LIBRARIES)
    cmake_parse_arguments(SPARKLE_COV "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    if (NOT SPARKLE_ENABLE_COVERAGE)
        return()
    endif()

    if (NOT SPARKLE_COV_TEST_TARGET)
        message(FATAL_ERROR "sparkle_enable_coverage requires TEST_TARGET <target>.")
    endif()

    if (NOT TARGET ${SPARKLE_COV_TEST_TARGET})
        message(FATAL_ERROR "sparkle_enable_coverage expected target '${SPARKLE_COV_TEST_TARGET}' to exist.")
    endif()

    if (MSVC)
        message(FATAL_ERROR "SPARKLE_ENABLE_COVERAGE requires Clang; MSVC does not provide the required coverage instrumentation.")
    endif()

    if (CMAKE_CXX_COMPILER_ID MATCHES "Clang")
        set(_sparkle_coverage_mode "llvm")
        set(_sparkle_cov_compile_flags -fprofile-instr-generate -fcoverage-mapping)
        set(_sparkle_cov_link_flags -fprofile-instr-generate -fcoverage-mapping)

        find_program(_sparkle_llvm_profdata_exe NAMES llvm-profdata)
        find_program(_sparkle_llvm_cov_exe NAMES llvm-cov)

        if (NOT _sparkle_llvm_profdata_exe OR NOT _sparkle_llvm_cov_exe)
            message(FATAL_ERROR "llvm-profdata and llvm-cov are required when SPARKLE_ENABLE_COVERAGE is ON.")
        endif()
    elseif (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
        message(FATAL_ERROR "SPARKLE_ENABLE_COVERAGE currently supports only Clang toolchains.")
    else()
        message(FATAL_ERROR "Unsupported compiler for coverage instrumentation: ${CMAKE_CXX_COMPILER_ID}")
    endif()

    if (NOT SPARKLE_BUILD_TESTS)
        message(FATAL_ERROR "SPARKLE_ENABLE_COVERAGE=ON requires SPARKLE_BUILD_TESTS=ON so that coverage can run the unit suite.")
    endif()

    find_package(Python3 COMPONENTS Interpreter REQUIRED)

    set(_sparkle_cov_dir "${CMAKE_BINARY_DIR}/coverage")
    set(_sparkle_cov_html_dir "${_sparkle_cov_dir}/html")
    set(_sparkle_cov_docs_dir "${CMAKE_SOURCE_DIR}/docs/build/coverage")

    set(_sparkle_cov_cmd
        ${Python3_EXECUTABLE}
        ${CMAKE_SOURCE_DIR}/tools/run_coverage.py
        --mode
        ${_sparkle_coverage_mode}
        --build-dir
        ${CMAKE_BINARY_DIR}
        --config
        $<CONFIG>
        --threshold
        ${SPARKLE_COVERAGE_MIN_LINE_RATE}
        --output
        ${_sparkle_cov_dir}/summary.txt
        --binary
        $<TARGET_FILE:${SPARKLE_COV_TEST_TARGET}>
        --source-root
        ${CMAKE_SOURCE_DIR}
        --html-dir
        ${_sparkle_cov_html_dir}
        --docs-dir
        ${_sparkle_cov_docs_dir}
        --project-name
        "Sparkle"
    )

    target_compile_options(${SPARKLE_COV_TEST_TARGET} PRIVATE ${_sparkle_cov_compile_flags})
    target_compile_definitions(${SPARKLE_COV_TEST_TARGET} PRIVATE SPARKLE_ENABLE_LLVM_COVERAGE=1)
    target_link_options(${SPARKLE_COV_TEST_TARGET} PRIVATE ${_sparkle_cov_link_flags})

    if (SPARKLE_COV_LIBRARIES)
        foreach(_sparkle_lib IN LISTS SPARKLE_COV_LIBRARIES)
            if (NOT TARGET ${_sparkle_lib})
                message(FATAL_ERROR "sparkle_enable_coverage expected library target '${_sparkle_lib}' to exist.")
            endif()
            target_compile_options(${_sparkle_lib} PRIVATE ${_sparkle_cov_compile_flags})
            list(APPEND _sparkle_cov_cmd --library $<TARGET_FILE:${_sparkle_lib}>)

            # Force the full archive into the test executable so that unreferenced
            # translation units still appear in the coverage report with 0% coverage.
            if (WIN32)
                # lld-link/MSVC-style flag to keep every object from the archive.
                target_link_options(${SPARKLE_COV_TEST_TARGET} PRIVATE
                    -Xlinker "/WHOLEARCHIVE:$<TARGET_FILE:${_sparkle_lib}>"
                )
            elseif (APPLE)
                # ld64 uses -force_load to pull every object from the archive.
                target_link_libraries(${SPARKLE_COV_TEST_TARGET} PRIVATE
                    "-Wl,-force_load,$<TARGET_FILE:${_sparkle_lib}>"
                )
            else()
                # GNU/LLD accept --whole-archive/--no-whole-archive to achieve the same.
                target_link_libraries(${SPARKLE_COV_TEST_TARGET} PRIVATE
                    "-Wl,--whole-archive"
                    $<TARGET_FILE:${_sparkle_lib}>
                    "-Wl,--no-whole-archive"
                )
            endif()
        endforeach()
    endif()

    list(APPEND _sparkle_cov_cmd --ignore-regex ".*/tests/.*")
    list(APPEND _sparkle_cov_cmd --ignore-regex ".*/_deps/.*")
    list(APPEND _sparkle_cov_cmd --ignore-regex ".*/vcpkg_installed/.*")
    list(APPEND _sparkle_cov_cmd --ignore-regex ".*[\\\\/]tests[\\/].*")
    list(APPEND _sparkle_cov_cmd --ignore-regex ".*[\\\\/]_deps[\\/].*")
    list(APPEND _sparkle_cov_cmd --ignore-regex ".*[\\\\/]vcpkg_installed[\\/].*")

    if (_sparkle_coverage_mode STREQUAL "llvm")
        list(APPEND _sparkle_cov_cmd
            --llvm-profdata
            ${_sparkle_llvm_profdata_exe}
            --llvm-cov
            ${_sparkle_llvm_cov_exe}
        )
    endif()

    add_custom_target(coverage
        COMMAND ${CMAKE_COMMAND} -E make_directory ${_sparkle_cov_dir}
        COMMAND ${_sparkle_cov_cmd}
        WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
        USES_TERMINAL
        COMMENT "Run unit tests and enforce coverage threshold"
    )

    add_dependencies(coverage ${SPARKLE_COV_TEST_TARGET})

    if (SPARKLE_COV_LIBRARIES)
        foreach(_sparkle_lib IN LISTS SPARKLE_COV_LIBRARIES)
            add_dependencies(coverage ${_sparkle_lib})
        endforeach()
    endif()
endfunction()
