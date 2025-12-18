find_package(rclcpp REQUIRED)

generate_ros_interface(TYPE_FOLDERS "types")

add_executable(ros_tests test_ros.cpp)
add_dependencies(ros_tests GenerateROSInterface)
add_subdirectory(ros)

target_link_libraries(ros_tests PRIVATE CoCo rclcpp::rclcpp)
target_compile_definitions(ros_tests PRIVATE PROJECT_ROOT="${CMAKE_CURRENT_SOURCE_DIR}")
setup_sanitizers(ros_tests)