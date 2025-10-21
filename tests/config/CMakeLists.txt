add_executable(coco_config coco_config.cpp)

set(COCO_CONFIG_TYPES types/parent.json types/child.json)
set(COCO_CONFIG_RULES rules/rule.clp)
coco_config("${COCO_CONFIG_TYPES}" "${COCO_CONFIG_RULES}")

add_dependencies(coco_config CoCo CoCoConfig)
target_include_directories(coco_config PRIVATE "${CoCo_INIT_INCLUDE_DIR}")
target_link_libraries(coco_config PRIVATE CoCo)
setup_sanitizers(coco_config)
