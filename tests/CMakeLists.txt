add_executable(coco_tests test_coco.cpp)
add_dependencies(coco_tests CoCo)
target_link_libraries(coco_tests PRIVATE CoCo)
setup_sanitizers(coco_tests)

add_executable(raise_tests test_raise.cpp)
add_dependencies(raise_tests CoCo)
target_link_libraries(raise_tests PRIVATE CoCo)
setup_sanitizers(raise_tests)

add_test(NAME COCOTest00 COMMAND coco_tests)
add_test(NAME RAISETest00 COMMAND raise_tests)