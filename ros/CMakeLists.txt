cmake_minimum_required(VERSION 3.15)
project(ROSGenerator LANGUAGES CXX)

add_executable(ROSGenerator main.cpp)
target_compile_features(ROSGenerator PUBLIC cxx_std_17)
target_include_directories(ROSGenerator PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>)
target_link_libraries(ROSGenerator PRIVATE json)
setup_sanitizers(ROSGenerator)

function(generate_ros_interface)
    message(STATUS "Generating ROS interface configuration")
    set(GEN_DIR ${CMAKE_BINARY_DIR}/ros_project)
    set(STAMP   ${GEN_DIR}/.generated.stamp)

    add_custom_command(
        OUTPUT ${STAMP}
        COMMAND ${CMAKE_COMMAND} -E make_directory ${GEN_DIR}
        COMMAND $<TARGET_FILE:ROSGenerator>
        DEPENDS ROSGenerator
        WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
        COMMENT "Generating ROS configuration header"
        VERBATIM
    )
endfunction()