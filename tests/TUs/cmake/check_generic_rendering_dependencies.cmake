file(GLOB_RECURSE GENERIC_RENDERING_HEADERS
    "${SPARKLE_SOURCE_ROOT}/includes/structures/graphics/rendering/pass/*.hpp"
    "${SPARKLE_SOURCE_ROOT}/includes/structures/graphics/rendering/plan/*.hpp")

set(FORBIDDEN_PATTERNS
    "structures/game_engine/"
    "ComponentRegistry"
    "ComponentLogicRegistry"
    "Camera3D"
    "SceneRenderPhase"
    "SceneRenderPassId"
    "SceneRenderFrameContext")

foreach(HEADER IN LISTS GENERIC_RENDERING_HEADERS)
    file(READ "${HEADER}" CONTENT)
    foreach(PATTERN IN LISTS FORBIDDEN_PATTERNS)
        string(FIND "${CONTENT}" "${PATTERN}" POSITION)
        if(NOT POSITION EQUAL -1)
            message(FATAL_ERROR "Generic rendering boundary violation in ${HEADER}: ${PATTERN}")
        endif()
    endforeach()
endforeach()
