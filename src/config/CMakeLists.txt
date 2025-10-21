cmake_minimum_required(VERSION 3.15)
project(CoCoGenerator LANGUAGES CXX)

add_executable(CoCoConfig config.cpp)
target_link_libraries(CoCoConfig json)
setup_sanitizers(CoCoConfig)

function(coco_config PROJECT_NAME TYPE_FILES RULE_FILES)
    set(GENERATED_DIR "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}_config")
    file(MAKE_DIRECTORY "${GENERATED_DIR}")
    set(GENERATED_HEADER "${GENERATED_DIR}/${PROJECT_NAME}_config.hpp")

    message(STATUS "Type files:")
    foreach(f IN LISTS TYPE_FILES)
        message(STATUS "${f}")
    endforeach()

    message(STATUS "Rule files:")
    foreach(f IN LISTS RULE_FILES)
        message(STATUS "${f}")
    endforeach()

    message(STATUS "CoCo configuration header " "${GENERATED_HEADER}")

    add_custom_command(
        OUTPUT "${GENERATED_HEADER}"
        COMMAND $<TARGET_FILE:CoCoConfig> -t ${TYPE_FILES} -r ${RULE_FILES} -o "${GENERATED_HEADER}"
        DEPENDS CoCoConfig ${TYPE_FILES} ${RULE_FILES}
        WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
        COMMENT "Generating CoCo configuration header for ${PROJECT_NAME}"
        VERBATIM
    )

    add_custom_target("${PROJECT_NAME}" ALL DEPENDS "${GENERATED_HEADER}")
    set(${PROJECT_NAME}_CONFIG_INCLUDE_DIR "${GENERATED_DIR}" PARENT_SCOPE)
endfunction()
