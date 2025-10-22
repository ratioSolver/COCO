add_executable(coco_config coco_config.cpp)

set(COCO_CONFIG_TYPES types/person.json types/adult.json types/man.json types/parent.json types/woman.json)
set(COCO_CONFIG_RULES rules/is_adult.clp)
coco_config(TestConfig "${COCO_CONFIG_TYPES}" "${COCO_CONFIG_RULES}")

add_dependencies(coco_config CoCo TestConfig)
target_include_directories(coco_config PRIVATE "${TestConfig_CONFIG_INCLUDE_DIR}")
target_link_libraries(coco_config PRIVATE CoCo)
setup_sanitizers(coco_config)
