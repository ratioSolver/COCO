add_executable(ros_tests test_ros.cpp)

target_link_libraries(ros_tests PRIVATE CoCo)
target_compile_definitions(ros_tests PRIVATE PROJECT_ROOT="${CMAKE_CURRENT_SOURCE_DIR}")
setup_sanitizers(ros_tests)

generate_ros_interface()