add_executable(coco_tests test_coco.cpp)
add_dependencies(coco_tests CoCo)
target_compile_definitions(coco_tests PRIVATE NUM_TESTS=1)
target_link_libraries(coco_tests PRIVATE CoCo)
setup_sanitizers(coco_tests)

add_test(NAME COCOTest00 COMMAND coco_tests)