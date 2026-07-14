find_package(GTest REQUIRED)

include(GoogleTest)

file(GLOB_RECURSE PLAYGROUND_TEST_SOURCES
	CONFIGURE_DEPENDS
	"${CMAKE_CURRENT_SOURCE_DIR}/*.cpp"
)

add_executable(PlaygroundTests ${PLAYGROUND_TEST_SOURCES})
add_dependencies(PlaygroundTests SparklePlayground)

# Tests run only on machines that have the checkout, so their main() pins the resource
# lookup straight at the live source tree — no deploy step, data edits apply instantly.
target_compile_definitions(PlaygroundTests
	PRIVATE
		PG_TEST_RESOURCE_DIR="${PROJECT_SOURCE_DIR}/resources"
)

# So a case can include shared fixtures as "support/<header>.hpp" from any subdirectory.
target_include_directories(PlaygroundTests PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}")

target_link_libraries(PlaygroundTests
	PRIVATE
		PlaygroundCore
		GTest::GTest
)
target_compile_features(PlaygroundTests PRIVATE cxx_std_23)

gtest_discover_tests(PlaygroundTests PROPERTIES LABELS playground)

foreach(INVALID_SIZE IN ITEMS -1 0 513)
	string(REPLACE "-" "negative_" TEST_SUFFIX "${INVALID_SIZE}")
	add_test(
		NAME "PlaygroundCliRejectsSize_${TEST_SUFFIX}"
		COMMAND
			${CMAKE_COMMAND}
			-DPROGRAM=$<TARGET_FILE:SparklePlayground>
			-DSIZE=${INVALID_SIZE}
			-DRESOURCES=${PROJECT_SOURCE_DIR}/resources
			-P "${CMAKE_CURRENT_SOURCE_DIR}/expect_cli_size_failure.cmake"
	)
	set_tests_properties(
		"PlaygroundCliRejectsSize_${TEST_SUFFIX}"
		PROPERTIES
			LABELS "playground;cli"
	)
endforeach()
