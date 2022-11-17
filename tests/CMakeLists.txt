add_executable(coco_tests test_coco.cpp)
target_link_libraries(coco_tests PRIVATE COCO oRatioSolver ratioCore RiDDLe SeMiTONE json)
target_compile_definitions(coco_tests PRIVATE COCO_ROOT="${COCO_ROOT}" MQTT_HOST="${MQTT_HOST}" MQTT_PORT="${MQTT_PORT}" MONGODB_HOST="${MONGODB_HOST}" MONGODB_PORT="${MONGODB_PORT}")

add_test(NAME COCOTest00 COMMAND coco_tests)