execute_process(
	COMMAND "${PROGRAM}" --map-only --size "${SIZE}"
	RESULT_VARIABLE result
	OUTPUT_VARIABLE output
	ERROR_VARIABLE error
)

if(result EQUAL 0)
	message(FATAL_ERROR "SparklePlayground unexpectedly accepted --size ${SIZE}")
endif()

set(combined "${output}${error}")
if(NOT combined MATCHES "WorldGenConfig\\.size")
	message(FATAL_ERROR "CLI did not report the shared size validation error: ${combined}")
endif()
