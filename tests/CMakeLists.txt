add_executable(coco_tests test_coco.cpp)
add_dependencies(coco_tests CoCo)
target_link_libraries(coco_tests PRIVATE CoCo)
setup_sanitizers(coco_tests)

add_executable(fcm_tests test_fcm.cpp)
add_dependencies(fcm_tests CoCo)
target_link_libraries(fcm_tests PRIVATE CoCo)
setup_sanitizers(fcm_tests)

add_test(NAME CoCoTest00 COMMAND coco_tests)
add_test(NAME FCMRAISETest00 COMMAND fcm_tests)