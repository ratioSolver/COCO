cmake_minimum_required(VERSION 3.15)
project(ROSInterfaceGenerator)

find_package(Python3 COMPONENTS Interpreter REQUIRED)

set(ROS_GENERATOR_SCRIPT ${CMAKE_CURRENT_SOURCE_DIR}/ros_generator.py CACHE FILEPATH "Path to the ROS interface generator script")

function(generate_ros_interface)
    set(options)
    set(oneValueArgs OUTPUT_DIR)
    set(multiValueArgs TYPE_FILES TYPE_FOLDERS)
    cmake_parse_arguments(COCO_ROS "" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    set(_ros_intf_cmd ${Python3_EXECUTABLE} ${ROS_GENERATOR_SCRIPT})
    if(COCO_ROS_TYPE_FILES)
        list(APPEND _ros_intf_cmd --type-files)
        foreach(_file IN LISTS COCO_ROS_TYPE_FILES)
            list(APPEND _ros_intf_cmd ${_file})
        endforeach()
    endif()
    if(COCO_ROS_TYPE_FOLDERS)
        list(APPEND _ros_intf_cmd --type-folders)
        foreach(_folder IN LISTS COCO_ROS_TYPE_FOLDERS)
            list(APPEND _ros_intf_cmd ${_folder})
        endforeach()
    endif()
    if(COCO_ROS_OUTPUT_DIR)
        list(APPEND _ros_intf_cmd --output-dir ${COCO_ROS_OUTPUT_DIR})
    else()
        list(APPEND _ros_intf_cmd --output-dir ros_interface)
    endif()

    execute_process(COMMAND ${_ros_intf_cmd}
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        RESULT_VARIABLE _result
        OUTPUT_VARIABLE _output
        ERROR_VARIABLE _error_output
    )
    if(NOT _result EQUAL 0)
        message(FATAL_ERROR "ROS interface generation failed with error:\n${_error_output}")
    else()
        message(STATUS "ROS interface generation succeeded:\n${_output}")
    endif()
endfunction()