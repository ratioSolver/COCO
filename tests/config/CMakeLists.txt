add_executable(coco_config coco_config.cpp)

target_link_libraries(coco_config PRIVATE CoCo)
target_compile_definitions(coco_config PRIVATE PROJECT_ROOT="${CMAKE_CURRENT_SOURCE_DIR}")
setup_sanitizers(coco_config)
