# The resource path is explicit: nothing is copied next to the executable any more, and
# without it the run aborts on the missing resource root before it ever reaches the size
# check this test is about.
execute_process(
	COMMAND "${PROGRAM}" --map-only --size "${SIZE}" --resource-path "${RESOURCES}"
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
