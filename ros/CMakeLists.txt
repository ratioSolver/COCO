cmake_minimum_required(VERSION 3.15)
project(ROSInterfaceGenerator)

find_package(Python3 COMPONENTS Interpreter REQUIRED)

set(ROS_INTERFACE_GENERATOR_SCRIPT ${CMAKE_CURRENT_SOURCE_DIR}/ros_interface_generator.py CACHE FILEPATH "Path to the ROS interface generator script")
set(COCO_ROS_GENERATOR_SCRIPT ${CMAKE_CURRENT_SOURCE_DIR}/coco_ros_generator.py CACHE FILEPATH "Path to the COCO ROS generator script")

function(generate_ros_interface)
    set(options)
    set(oneValueArgs OUTPUT_DIR)
    set(multiValueArgs TYPE_FILES TYPE_FOLDERS)
    cmake_parse_arguments(COCO_ROS "" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    set(_ros_intf_cmd ${Python3_EXECUTABLE} ${ROS_INTERFACE_GENERATOR_SCRIPT})
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

    set(_coco_ros_gen_cmd ${Python3_EXECUTABLE} ${COCO_ROS_GENERATOR_SCRIPT})
    if(COCO_ROS_TYPE_FILES)
        list(APPEND _coco_ros_gen_cmd --type-files)
        foreach(_file IN LISTS COCO_ROS_TYPE_FILES)
            list(APPEND _coco_ros_gen_cmd ${_file})
        endforeach()
    endif()
    if(COCO_ROS_TYPE_FOLDERS)
        list(APPEND _coco_ros_gen_cmd --type-folders)
        foreach(_folder IN LISTS COCO_ROS_TYPE_FOLDERS)
            list(APPEND _coco_ros_gen_cmd ${_folder})
        endforeach()
    endif()
    if(COCO_ROS_OUTPUT_DIR)
        list(APPEND _coco_ros_gen_cmd --output-dir ${COCO_ROS_OUTPUT_DIR})
    else()
        list(APPEND _coco_ros_gen_cmd --output-dir ros_interface)
    endif()

    execute_process(COMMAND ${_coco_ros_gen_cmd}
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        RESULT_VARIABLE _coco_result
        OUTPUT_VARIABLE _coco_output
        ERROR_VARIABLE _coco_error_output
    )
    if(NOT _coco_result EQUAL 0)
        message(FATAL_ERROR "COCO ROS generation failed with error:\n${_coco_error_output}")
    else()
        message(STATUS "COCO ROS generation succeeded:\n${_coco_output}")
    endif()
endfunction()