generate_ros_interface(TYPE_FOLDERS "types")
add_subdirectory(coco_ros)

add_executable(ros_tests test_ros.cpp)

target_link_libraries(ros_tests PRIVATE coco_ros_lib)
target_compile_definitions(ros_tests PRIVATE PROJECT_ROOT="${CMAKE_CURRENT_SOURCE_DIR}")
setup_sanitizers(ros_tests)