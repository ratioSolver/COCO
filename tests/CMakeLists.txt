add_executable(coco_tests test_coco.cpp)
add_dependencies(coco_tests CoCo)
target_link_libraries(coco_tests PRIVATE CoCo)
setup_sanitizers(coco_tests)

add_executable(raise_tests test_raise.cpp)
add_dependencies(raise_tests CoCo)
target_link_libraries(raise_tests PRIVATE CoCo)
setup_sanitizers(raise_tests)

if(DEFINED ENV{GITHUB_ACTIONS})
    message(STATUS "GitHub action..")

    target_compile_definitions(coco_tests PUBLIC NO_INTERACTIVE_MODE)
    target_compile_definitions(raise_tests PUBLIC NO_INTERACTIVE_MODE)
endif()

add_test(NAME COCOTest00 COMMAND coco_tests)
add_test(NAME RAISETest00 COMMAND raise_tests)