cmake_minimum_required(VERSION 3.15)
project(ROSInterfaceGenerator)

find_package(Python3 COMPONENTS Interpreter REQUIRED)

function(generate_ros_interface)
    set(options)
    set(oneValueArgs OUTPUT_DIR)
    set(multiValueArgs TYPE_FILES TYPE_FOLDERS)
    cmake_parse_arguments(COCO_ROS "" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})
endfunction()