add_executable(ros_tests test_ros.cpp)

find_package(ament_cmake REQUIRED)
find_package(rclcpp REQUIRED)

generate_ros_interface()

target_link_libraries(ros_tests PRIVATE CoCo rclcpp::rclcpp)
target_include_directories(ros_tests PRIVATE ${ROS_GEN_DIR})
target_compile_definitions(ros_tests PRIVATE PROJECT_ROOT="${CMAKE_CURRENT_SOURCE_DIR}")
setup_sanitizers(ros_tests)