cmake_minimum_required(VERSION 3.15)
project(ROSGenerator LANGUAGES CXX)

add_executable(ROSGenerator main.cpp)
target_compile_features(ROSGenerator PUBLIC cxx_std_17)
target_include_directories(ROSGenerator PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>)
target_link_libraries(ROSGenerator PRIVATE json)
setup_sanitizers(ROSGenerator)

function(generate_ros_interface)
    set(multiValueArgs TYPE_FILES TYPE_FOLDERS)
    cmake_parse_arguments(COCO_ROS "" "" "${multiValueArgs}" ${ARGN})

    message(STATUS "Generating ROS interface configuration")
    set(ROS_GEN_DIR ${CMAKE_CURRENT_SOURCE_DIR}/ros)
    set(ROS_GEN_CPP ${ROS_GEN_DIR}/coco_ros.cpp)
    set(ROS_GEN_HPP ${ROS_GEN_DIR}/coco_ros.hpp)
    set(ROS_GEN_PKG ${ROS_GEN_DIR}/package.xml)

    message(STATUS "Type files:")
    foreach(f IN LISTS COCO_ROS_TYPE_FILES)
        message(STATUS "${f}")
    endforeach()

    message(STATUS "Type folders:")
    foreach(f IN LISTS COCO_ROS_TYPE_FOLDERS)
        message(STATUS "${f}")
    endforeach()

    set(ROS_GEN_DEPENDS)
    foreach(tp_file IN LISTS COCO_ROS_TYPE_FILES)
        if(IS_ABSOLUTE "${tp_file}")
            list(APPEND ROS_GEN_DEPENDS "${tp_file}")
        else()
            list(APPEND ROS_GEN_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/${tp_file}")
        endif()
    endforeach()

    foreach(tp_folder IN LISTS COCO_ROS_TYPE_FOLDERS)
        if(IS_ABSOLUTE "${tp_folder}")
            set(_folder_path "${tp_folder}")
        else()
            set(_folder_path "${CMAKE_CURRENT_SOURCE_DIR}/${tp_folder}")
        endif()
        file(GLOB_RECURSE _folder_types CONFIGURE_DEPENDS LIST_DIRECTORIES FALSE
            "${_folder_path}/*")
        list(APPEND ROS_GEN_DEPENDS ${_folder_types})
    endforeach()

    add_custom_command(
        OUTPUT ${ROS_GEN_CPP} ${ROS_GEN_HPP} ${ROS_GEN_PKG}
        COMMAND ${CMAKE_COMMAND} -E make_directory ${ROS_GEN_DIR}
        COMMAND $<TARGET_FILE:ROSGenerator>
            -t ${COCO_ROS_TYPE_FILES}
            -tf ${COCO_ROS_TYPE_FOLDERS}
            -o ${ROS_GEN_DIR}
        DEPENDS ROSGenerator ${ROS_GEN_DEPENDS}
        WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
        COMMENT "Generating ROS configuration header"
        VERBATIM
    )
    set_source_files_properties(${ROS_GEN_CPP} ${ROS_GEN_HPP} PROPERTIES GENERATED TRUE)
    add_custom_target(GenerateROSInterface ALL DEPENDS ${ROS_GEN_CPP} ${ROS_GEN_HPP} ${ROS_GEN_PKG})
    set(ROS_GEN_DIR ${ROS_GEN_DIR} PARENT_SCOPE)
endfunction()