include_guard(GLOBAL)

function(sparkle_enable_coverage)
    set(options)
    set(oneValueArgs TEST_TARGET)
    set(multiValueArgs LIBRARIES)
    cmake_parse_arguments(SPARKLE_COV "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    if(NOT SPARKLE_ENABLE_COVERAGE)
        return()
    endif()

    if(NOT SPARKLE_COV_TEST_TARGET)
        message(FATAL_ERROR "sparkle_enable_coverage requires TEST_TARGET <target>.")
    endif()

    if(NOT TARGET ${SPARKLE_COV_TEST_TARGET})
        message(FATAL_ERROR "sparkle_enable_coverage expected target '${SPARKLE_COV_TEST_TARGET}' to exist.")
    endif()

    if(MSVC)
        message(FATAL_ERROR "SPARKLE_ENABLE_COVERAGE requires Clang; MSVC does not provide the required coverage instrumentation.")
    endif()

    if(NOT CMAKE_CXX_COMPILER_ID MATCHES "Clang")
        message(FATAL_ERROR "SPARKLE_ENABLE_COVERAGE currently supports only Clang toolchains.")
    endif()

    if(NOT SPARKLE_BUILD_TESTS)
        message(FATAL_ERROR "SPARKLE_ENABLE_COVERAGE=ON requires SPARKLE_BUILD_TESTS=ON so that coverage can run the unit suite.")
    endif()

    find_program(_sparkle_llvm_profdata_exe NAMES llvm-profdata REQUIRED)
    find_program(_sparkle_llvm_cov_exe NAMES llvm-cov REQUIRED)

    set(_sparkle_cov_compile_flags -O0 -g -fprofile-instr-generate -fcoverage-mapping)
    set(_sparkle_cov_link_flags -fprofile-instr-generate -fcoverage-mapping)

    target_compile_options(${SPARKLE_COV_TEST_TARGET} PRIVATE ${_sparkle_cov_compile_flags})
    target_compile_definitions(${SPARKLE_COV_TEST_TARGET} PRIVATE SPARKLE_ENABLE_LLVM_COVERAGE=1)
    target_link_options(${SPARKLE_COV_TEST_TARGET} PRIVATE ${_sparkle_cov_link_flags})

    if(SPARKLE_COV_LIBRARIES)
        foreach(_sparkle_lib IN LISTS SPARKLE_COV_LIBRARIES)
            if(NOT TARGET ${_sparkle_lib})
                message(FATAL_ERROR "sparkle_enable_coverage expected library target '${_sparkle_lib}' to exist.")
            endif()

            target_compile_options(${_sparkle_lib} PRIVATE ${_sparkle_cov_compile_flags})

            if(WIN32)
                target_link_options(${SPARKLE_COV_TEST_TARGET} PRIVATE
                    -Xlinker "/WHOLEARCHIVE:$<TARGET_FILE:${_sparkle_lib}>"
                )
            elseif(APPLE)
                target_link_libraries(${SPARKLE_COV_TEST_TARGET} PRIVATE
                    "-Wl,-force_load,$<TARGET_FILE:${_sparkle_lib}>"
                )
            else()
                target_link_libraries(${SPARKLE_COV_TEST_TARGET} PRIVATE
                    "-Wl,--whole-archive"
                    $<TARGET_FILE:${_sparkle_lib}>
                    "-Wl,--no-whole-archive"
                )
            endif()
        endforeach()
    endif()

    add_custom_target(coverage
        COMMAND
            ${CMAKE_COMMAND}
            "-DLLVM_COV_EXECUTABLE=${_sparkle_llvm_cov_exe}"
            "-DLLVM_PROFDATA_EXECUTABLE=${_sparkle_llvm_profdata_exe}"
            "-DBINARY_DIR=${CMAKE_BINARY_DIR}"
            "-DSOURCE_DIR=${CMAKE_SOURCE_DIR}"
            "-DTEST_EXECUTABLE=$<TARGET_FILE:${SPARKLE_COV_TEST_TARGET}>"
            -P "${CMAKE_SOURCE_DIR}/cmake/run_llvm_coverage.cmake"
        WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
        USES_TERMINAL
        COMMENT "Run unit tests and generate LLVM coverage report"
    )

    add_dependencies(coverage ${SPARKLE_COV_TEST_TARGET})

    if(SPARKLE_COV_LIBRARIES)
        foreach(_sparkle_lib IN LISTS SPARKLE_COV_LIBRARIES)
            add_dependencies(coverage ${_sparkle_lib})
        endforeach()
    endif()
endfunction()
