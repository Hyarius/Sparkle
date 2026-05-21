foreach(required_var
    LLVM_COV_EXECUTABLE
    LLVM_PROFDATA_EXECUTABLE
    BINARY_DIR
    SOURCE_DIR
    TEST_EXECUTABLE
)
    if(NOT DEFINED ${required_var})
        message(FATAL_ERROR "Missing required coverage variable: ${required_var}")
    endif()
endforeach()

set(PROFILE_PATTERN "${BINARY_DIR}/sparkle-%p.profraw")
set(PROFDATA_FILE "${BINARY_DIR}/coverage.profdata")
set(REPORT_FILE "${BINARY_DIR}/coverage-report.txt")
set(SUMMARY_FILE "${BINARY_DIR}/coverage-summary.json")
set(HTML_DIR "${BINARY_DIR}/coverage-html")

file(GLOB old_profiles "${BINARY_DIR}/sparkle-*.profraw")
if(old_profiles)
    file(REMOVE ${old_profiles})
endif()
file(GLOB old_intermediate_profiles "${BINARY_DIR}/coverage-chunk-*.profdata")
if(old_intermediate_profiles)
    file(REMOVE ${old_intermediate_profiles})
endif()
file(REMOVE "${PROFDATA_FILE}" "${REPORT_FILE}" "${SUMMARY_FILE}")
file(REMOVE_RECURSE "${HTML_DIR}")

execute_process(
    COMMAND
        "${CMAKE_COMMAND}" -E env "LLVM_PROFILE_FILE=${PROFILE_PATTERN}"
        "${CMAKE_CTEST_COMMAND}" --test-dir "${BINARY_DIR}" --output-on-failure
    RESULT_VARIABLE test_result
)
if(NOT test_result EQUAL 0)
    message(WARNING "Tests failed; coverage will still be generated from the produced profile data")
endif()

file(GLOB profile_files "${BINARY_DIR}/sparkle-*.profraw")
if(NOT profile_files)
    message(FATAL_ERROR "No LLVM profile data was produced")
endif()

set(intermediate_profiles)
set(profile_chunk)
set(chunk_index 0)
foreach(profile_file IN LISTS profile_files)
    list(APPEND profile_chunk "${profile_file}")
    list(LENGTH profile_chunk profile_chunk_size)

    if(profile_chunk_size GREATER_EQUAL 100)
        set(chunk_output "${BINARY_DIR}/coverage-chunk-${chunk_index}.profdata")
        execute_process(
            COMMAND
                "${LLVM_PROFDATA_EXECUTABLE}" merge -sparse ${profile_chunk}
                -o "${chunk_output}"
            RESULT_VARIABLE merge_result
        )
        if(NOT merge_result EQUAL 0)
            message(FATAL_ERROR "Failed to merge LLVM profile data chunk ${chunk_index}")
        endif()

        list(APPEND intermediate_profiles "${chunk_output}")
        set(profile_chunk)
        math(EXPR chunk_index "${chunk_index} + 1")
    endif()
endforeach()

if(profile_chunk)
    set(chunk_output "${BINARY_DIR}/coverage-chunk-${chunk_index}.profdata")
    execute_process(
        COMMAND
            "${LLVM_PROFDATA_EXECUTABLE}" merge -sparse ${profile_chunk}
            -o "${chunk_output}"
        RESULT_VARIABLE merge_result
    )
    if(NOT merge_result EQUAL 0)
        message(FATAL_ERROR "Failed to merge LLVM profile data chunk ${chunk_index}")
    endif()

    list(APPEND intermediate_profiles "${chunk_output}")
endif()

list(LENGTH intermediate_profiles intermediate_profile_count)
if(intermediate_profile_count EQUAL 1)
    execute_process(
        COMMAND "${CMAKE_COMMAND}" -E copy "${intermediate_profiles}" "${PROFDATA_FILE}"
        RESULT_VARIABLE merge_result
    )
    if(NOT merge_result EQUAL 0)
        message(FATAL_ERROR "Failed to copy LLVM profile data")
    endif()
else()
    execute_process(
        COMMAND
            "${LLVM_PROFDATA_EXECUTABLE}" merge -sparse ${intermediate_profiles}
            -o "${PROFDATA_FILE}"
        RESULT_VARIABLE merge_result
    )
    if(NOT merge_result EQUAL 0)
        message(FATAL_ERROR "Failed to merge LLVM profile data")
    endif()
endif()

set(IGNORE_PATTERNS
    ".*[/\\\\]tests[/\\\\].*"
    ".*[/\\\\]build[/\\\\].*"
    ".*[/\\\\]vcpkg_installed[/\\\\].*"
)

set(IGNORE_ARGS)
foreach(pattern IN LISTS IGNORE_PATTERNS)
    list(APPEND IGNORE_ARGS "-ignore-filename-regex=${pattern}")
endforeach()

execute_process(
    COMMAND
        "${LLVM_COV_EXECUTABLE}" report "${TEST_EXECUTABLE}"
        "-instr-profile=${PROFDATA_FILE}"
        ${IGNORE_ARGS}
    RESULT_VARIABLE report_result
    OUTPUT_VARIABLE coverage_report
    ERROR_VARIABLE coverage_error
)
if(NOT report_result EQUAL 0)
    message("${coverage_error}")
    message(FATAL_ERROR "Failed to generate LLVM coverage report")
endif()

message("${coverage_report}")
file(WRITE "${REPORT_FILE}" "${coverage_report}")

execute_process(
    COMMAND
        "${LLVM_COV_EXECUTABLE}" export "${TEST_EXECUTABLE}"
        "-instr-profile=${PROFDATA_FILE}"
        ${IGNORE_ARGS}
        "-format=text"
    RESULT_VARIABLE export_result
    OUTPUT_FILE "${SUMMARY_FILE}"
    ERROR_VARIABLE export_error
)
if(NOT export_result EQUAL 0)
    message("${export_error}")
    message(FATAL_ERROR "Failed to export LLVM coverage summary")
endif()

execute_process(
    COMMAND
        "${LLVM_COV_EXECUTABLE}" show "${TEST_EXECUTABLE}"
        "-instr-profile=${PROFDATA_FILE}"
        ${IGNORE_ARGS}
        "-format=html"
        "-output-dir=${HTML_DIR}"
        "-show-line-counts-or-regions"
        "-show-branches=count"
    RESULT_VARIABLE html_result
    ERROR_VARIABLE html_error
)
if(NOT html_result EQUAL 0)
    message("${html_error}")
    message(FATAL_ERROR "Failed to generate LLVM coverage HTML report")
endif()

message(STATUS "Coverage file summary written to ${REPORT_FILE}")
message(STATUS "Coverage summary written to ${SUMMARY_FILE}")
message(STATUS "Coverage annotated HTML written to ${HTML_DIR}/index.html")

if(NOT test_result EQUAL 0)
    message(FATAL_ERROR "Coverage report was generated, but one or more tests failed")
endif()
