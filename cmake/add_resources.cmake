function(add_resources)
    if(NOT TARGET sparkleResourcesConverter)
        message(FATAL_ERROR "add_resources requires the sparkleResourcesConverter target to exist.")
    endif()

    set(_resource_files ${ARGV})
    set(_generated_header "${CMAKE_BINARY_DIR}/spk_generated_resources.hpp")
    set(_base_dir "${CMAKE_SOURCE_DIR}")
    set(_resource_depends)

    foreach(_resource_file IN LISTS _resource_files)
        if(IS_ABSOLUTE "${_resource_file}")
            list(APPEND _resource_depends "${_resource_file}")
        else()
            list(APPEND _resource_depends "${CMAKE_SOURCE_DIR}/${_resource_file}")
        endif()
    endforeach()

    add_custom_command(
        OUTPUT "${_generated_header}"
        COMMAND $<TARGET_FILE:sparkleResourcesConverter>
                "${_generated_header}"
                "${_base_dir}"
                ${_resource_files}
        DEPENDS sparkleResourcesConverter ${_resource_depends}
        COMMENT "Generating embedded resource header: ${_generated_header}"
        VERBATIM
    )

    add_custom_target(SparkleResourcesHeaderTarget
        DEPENDS "${_generated_header}"
    )

    set(SPARKLE_RESOURCES_HEADER "${_generated_header}" PARENT_SCOPE)
endfunction()
