cmake_minimum_required(VERSION 3.15)
project(CoCoGenerator LANGUAGES CXX)

add_executable(ConfigGenerator main.cpp src/config_generator.cpp)
target_compile_features(ConfigGenerator PUBLIC cxx_std_17)
target_include_directories(ConfigGenerator PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>)
target_link_libraries(ConfigGenerator PRIVATE json)
setup_sanitizers(ConfigGenerator)

function(coco_config)
    set(options ROS_INTERFACE)
    set(oneValueArgs PROJ_NAME)
    set(multiValueArgs TYPE_FILES TYPE_FOLDERS RULE_FILES RULE_FOLDERS ITEM_FILES ITEM_FOLDERS)
    cmake_parse_arguments(COCO_CONFIG "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    set(GENERATED_DIR "${CMAKE_CURRENT_BINARY_DIR}/${COCO_CONFIG_PROJ_NAME}_config")
    file(MAKE_DIRECTORY "${GENERATED_DIR}")
    set(GENERATED_HEADER "${GENERATED_DIR}/${COCO_CONFIG_PROJ_NAME}_config.hpp")

    message(STATUS "Type files:")
    foreach(f IN LISTS COCO_CONFIG_TYPE_FILES)
        message(STATUS "${f}")
    endforeach()

    message(STATUS "Rule files:")
    foreach(f IN LISTS COCO_CONFIG_RULE_FILES)
        message(STATUS "${f}")
    endforeach()

    message(STATUS "Item files:")
    foreach(f IN LISTS COCO_CONFIG_ITEM_FILES)
        message(STATUS "${f}")
    endforeach()

    message(STATUS "CoCo configuration header " "${GENERATED_HEADER}")

    add_custom_command(
        OUTPUT "${GENERATED_HEADER}"
        COMMAND $<TARGET_FILE:ConfigGenerator>
            -t ${COCO_CONFIG_TYPE_FILES}
            -tf ${COCO_CONFIG_TYPE_FOLDERS}
            -r ${COCO_CONFIG_RULE_FILES}
            -rf ${COCO_CONFIG_RULE_FOLDERS}
            -i ${COCO_CONFIG_ITEM_FILES}
            -if ${COCO_CONFIG_ITEM_FOLDERS}
            -o "${GENERATED_HEADER}"
        DEPENDS ConfigGenerator
            ${COCO_CONFIG_TYPE_FILES}
            ${COCO_CONFIG_RULE_FILES}
            ${COCO_CONFIG_ITEM_FILES}
        WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
        COMMENT "Generating CoCo configuration header for ${COCO_CONFIG_PROJ_NAME}"
        VERBATIM
    )

    add_custom_target("${COCO_CONFIG_PROJ_NAME}" ALL DEPENDS "${GENERATED_HEADER}")
    set(${COCO_CONFIG_PROJ_NAME}_CONFIG_INCLUDE_DIR "${GENERATED_DIR}" PARENT_SCOPE)
endfunction()
