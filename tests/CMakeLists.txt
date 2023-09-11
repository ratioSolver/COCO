add_executable(coco_tests test_coco.cpp)
target_link_libraries(coco_tests PRIVATE COCO ${CMAKE_THREAD_LIBS_INIT} PahoMqttCpp::paho-mqttpp3-static clips mongo::bsoncxx_shared mongo::mongocxx_shared)
target_compile_definitions(coco_tests PRIVATE COCO_APP="${COCO_APP}" COCO_INSTANCE="${COCO_INSTANCE}" MQTT_APP_HOST="${MQTT_APP_HOST}" MQTT_APP_PORT="${MQTT_APP_PORT}" MQTT_INSTANCE_HOST="${MQTT_INSTANCE_HOST}" MQTT_INSTANCE_PORT="${MQTT_INSTANCE_PORT}" MONGODB_APP_HOST="${MONGODB_APP_HOST}" MONGODB_APP_PORT="${MONGODB_APP_PORT}" MONGODB_INSTANCE_HOST="${MONGODB_INSTANCE_HOST}" MONGODB_INSTANCE_PORT="${MONGODB_INSTANCE_PORT}")

add_test(NAME COCOTest00 COMMAND coco_tests)