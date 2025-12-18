cmake_minimum_required(VERSION 3.15)
project(ROSGenerator LANGUAGES CXX)

add_executable(ROSGenerator main.cpp)
target_compile_features(ROSGenerator PUBLIC cxx_std_17)
target_include_directories(ROSGenerator PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>)
target_link_libraries(ROSGenerator PRIVATE json)
setup_sanitizers(ROSGenerator)

function(generate_ros_interface)
    message(STATUS "Generating ROS interface configuration")
    set(ROS_GEN_DIR ${CMAKE_CURRENT_SOURCE_DIR}/ros)
    set(STAMP   ${ROS_GEN_DIR}/.generated.stamp)

    add_custom_command(
        OUTPUT ${STAMP}
        COMMAND ${CMAKE_COMMAND} -E make_directory ${ROS_GEN_DIR}
        COMMAND $<TARGET_FILE:ROSGenerator> -o ${ROS_GEN_DIR}
        DEPENDS ROSGenerator
        WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
        COMMENT "Generating ROS configuration header"
        VERBATIM
    )
    add_custom_target(GenerateROSInterface ALL DEPENDS ${STAMP})
    set(ROS_GEN_DIR ${ROS_GEN_DIR} PARENT_SCOPE)
endfunction()