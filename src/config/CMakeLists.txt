cmake_minimum_required(VERSION 3.15)
project(CoCoGenerator LANGUAGES CXX)

add_executable(CoCoConfig config.cpp)
target_link_libraries(CoCoConfig json)

function(coco_config TYPE_FILES RULE_FILES)
    set(GENERATED_DIR "${CMAKE_CURRENT_BINARY_DIR}/${COCO_NAME}_init")
    file(MAKE_DIRECTORY "${GENERATED_DIR}")
    set(GENERATED_HEADER "${GENERATED_DIR}/${COCO_NAME}_init.hpp")

    message(STATUS "Type files:")
    set(ABS_TYPES)
    foreach(f IN LISTS TYPE_FILES)
        if(NOT IS_ABSOLUTE "${f}")
            get_filename_component(a "${CMAKE_CURRENT_SOURCE_DIR}/${f}" ABSOLUTE)
        else()
            set(a "${f}")
        endif()
        list(APPEND ABS_TYPES "${a}")
        message(STATUS "${a}")
    endforeach()

    message(STATUS "Rule files:")
    set(ABS_RULES)
    foreach(f IN LISTS RULE_FILES)
        if(NOT IS_ABSOLUTE "${f}")
            get_filename_component(a "${CMAKE_CURRENT_SOURCE_DIR}/${f}" ABSOLUTE)
        else()
            set(a "${f}")
        endif()
        list(APPEND ABS_RULES "${a}")
        message(STATUS "${a}")
    endforeach()

    message(STATUS "Output file: " "${GENERATED_HEADER}")

    add_custom_command(
        OUTPUT "${GENERATED_HEADER}"
        COMMAND $<TARGET_FILE:CoCoConfig> -t ${ABS_TYPES} -r ${ABS_RULES} -o "${GENERATED_HEADER}"
        DEPENDS CoCoConfig ${ABS_TYPES} ${ABS_RULES}
        WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
        COMMENT "Generating CoCo configuration header for ${COCO_NAME}"
        VERBATIM
    )

    add_custom_target("${COCO_NAME}_config" ALL DEPENDS "${GENERATED_HEADER}")
    set(${COCO_NAME}_INIT_INCLUDE_DIR "${GENERATED_DIR}" PARENT_SCOPE)
endfunction()
