cmake_minimum_required(VERSION 3.15)
project(CoCoGenerator LANGUAGES CXX)

add_executable(ConfigGenerator main.cpp src/config_generator.cpp)
target_compile_features(ConfigGenerator PUBLIC cxx_std_17)
target_include_directories(ConfigGenerator PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>)
target_link_libraries(ConfigGenerator PRIVATE json)
setup_sanitizers(ConfigGenerator)

function(coco_config PROJECT_NAME TYPE_FILES RULE_FILES ITEM_FILES)
    set(GENERATED_DIR "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}_config")
    file(MAKE_DIRECTORY "${GENERATED_DIR}")
    set(GENERATED_HEADER "${GENERATED_DIR}/${PROJECT_NAME}_config.hpp")

    if(NOT TYPE_FILES)
        set(TYPE_FILES "")
    endif()
    if(NOT RULE_FILES)
        set(RULE_FILES "")
    endif()
    if(NOT ITEM_FILES)
        set(ITEM_FILES "")
    endif()

    message(STATUS "Type files:")
    foreach(f IN LISTS TYPE_FILES)
        message(STATUS "${f}")
    endforeach()

    message(STATUS "Rule files:")
    foreach(f IN LISTS RULE_FILES)
        message(STATUS "${f}")
    endforeach()

    message(STATUS "Item files:")
    foreach(f IN LISTS ITEM_FILES)
        message(STATUS "${f}")
    endforeach()

    message(STATUS "CoCo configuration header " "${GENERATED_HEADER}")

    add_custom_command(
        OUTPUT "${GENERATED_HEADER}"
        COMMAND $<TARGET_FILE:ConfigGenerator> -t ${TYPE_FILES} -r ${RULE_FILES} -o "${GENERATED_HEADER}" -i ${ITEM_FILES}
        DEPENDS ConfigGenerator ${TYPE_FILES} ${RULE_FILES}
        WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
        COMMENT "Generating CoCo configuration header for ${PROJECT_NAME}"
        VERBATIM
    )

    add_custom_target("${PROJECT_NAME}" ALL DEPENDS "${GENERATED_HEADER}")
    set(${PROJECT_NAME}_CONFIG_INCLUDE_DIR "${GENERATED_DIR}" PARENT_SCOPE)
endfunction()
