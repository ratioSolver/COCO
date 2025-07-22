add_executable(coco_tests test_coco.cpp)
add_dependencies(coco_tests CoCo)
target_link_libraries(coco_tests PRIVATE CoCo)
setup_sanitizers(coco_tests)

add_executable(robot_tests test_robot.cpp)
add_dependencies(robot_tests CoCo)
target_link_libraries(robot_tests PRIVATE CoCo)
setup_sanitizers(robot_tests)

add_executable(raise_tests test_raise.cpp)
add_dependencies(raise_tests CoCo)
target_link_libraries(raise_tests PRIVATE CoCo)
setup_sanitizers(raise_tests)

add_executable(restart_tests test_restart.cpp)
add_dependencies(restart_tests CoCo)
target_link_libraries(restart_tests PRIVATE CoCo)
setup_sanitizers(restart_tests)

add_test(NAME COCOTest00 COMMAND coco_tests)
add_test(NAME RobotTest00 COMMAND robot_tests)
add_test(NAME RAISETest00 COMMAND raise_tests)
add_test(NAME RESTARTTest00 COMMAND restart_tests)