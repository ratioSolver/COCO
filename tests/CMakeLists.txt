add_executable(coco_tests test_coco.cpp)
target_link_libraries(coco_tests PRIVATE COCO ${CMAKE_THREAD_LIBS_INIT} clips mongo::bsoncxx_shared mongo::mongocxx_shared)
target_compile_definitions(coco_tests PRIVATE COCO_NAME="${COCO_NAME}")

if(MONGO_DB)
    target_compile_definitions(coco_tests PRIVATE MONGODB_HOST="${MONGODB_HOST}" MONGODB_PORT="${MONGODB_PORT}")
endif()

add_test(NAME COCOTest00 COMMAND coco_tests)