cmake_minimum_required(VERSION 3.21)
project(CoCo VERSION 0.2.0 LANGUAGES CXX)

include(CTest)
enable_testing()

set(COCO_NAME "CoCo" CACHE STRING "The CoCo Application Name")

set(CLIPS_INCLUDE_DIR /usr/local/include/clips CACHE PATH "CLIPS include directory")
set(CLIPS_LIB_DIR /usr/local/lib CACHE PATH "CLIPS library directory")

option(BUILD_MONGODB "Build MongoDB drivers" ON)
option(BUILD_COCO_SERVER "Build the CoCo server" OFF)
option(BUILD_MQTT "Build MQTT middleware" OFF)

message(STATUS "CoCo name: ${COCO_NAME}")

add_library(CoCo src/coco.cpp src/coco_property.cpp src/coco_type.cpp src/coco_item.cpp src/coco_db.cpp)
target_compile_features(CoCo PUBLIC cxx_std_17)
target_include_directories(CoCo PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include> ${CLIPS_INCLUDE_DIR})
if(NOT TARGET json)
    add_subdirectory(extern/json)
endif()
add_dependencies(CoCo json)
target_link_directories(CoCo PUBLIC ${CLIPS_LIB_DIR})
target_link_libraries(CoCo PUBLIC json clips)
target_compile_definitions(CoCo PUBLIC COCO_NAME="${COCO_NAME}")
setup_sanitizers(CoCo)

message(STATUS "Build MongoDB drivers: ${BUILD_MONGODB}")
if(BUILD_MONGODB)
    find_package(mongocxx REQUIRED)
    find_package(bsoncxx REQUIRED)
    
    set(MONGODB_HOST "localhost" CACHE STRING "The MongoDB Host")
    set(MONGODB_PORT "27017" CACHE STRING "The MongoDB Port")

    message(STATUS "MongoDB host: ${MONGODB_HOST}")
    message(STATUS "MongoDB port: ${MONGODB_PORT}")

    target_sources(CoCo PRIVATE src/db/mongo/mongo_db.cpp)
    target_include_directories(CoCo PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include/db/mongo> ${LIBMONGOCXX_INCLUDE_DIR} ${LIBBSONCXX_INCLUDE_DIR})
    target_link_libraries(CoCo PUBLIC mongo::bsoncxx_shared mongo::mongocxx_shared)
    target_compile_definitions(CoCo PUBLIC BUILD_MONGODB MONGODB_HOST="${MONGODB_HOST}" MONGODB_PORT="${MONGODB_PORT}")
endif()

message(STATUS "Build CoCo server: ${BUILD_COCO_SERVER}")
if(BUILD_COCO_SERVER)
    if(NOT TARGET ratioNet)
        add_subdirectory(extern/rationet)
    endif()

    add_dependencies(CoCo ratioNet)
    target_link_libraries(CoCo PUBLIC ratioNet)
    target_compile_definitions(CoCo PUBLIC BUILD_LISTENERS)
endif()

message(STATUS "Build MQTT middleware: ${BUILD_MQTT}")
if(BUILD_MQTT)
    find_package(PahoMqttCpp REQUIRED)

    set(MQTT_HOST "localhost" CACHE STRING "The MQTT Host")
    set(MQTT_PORT "1883" CACHE STRING "The MQTT Port")

    message(STATUS "MQTT host: ${MQTT_HOST}")
    message(STATUS "MQTT port: ${MQTT_PORT}")

    target_include_directories(CoCo PUBLIC ${PAHO_MQTT_CPP_INCLUDE_DIR})
    target_link_libraries(CoCo PUBLIC ${PAHO_MQTT_CPP_LIBRARIES})
    target_compile_definitions(CoCo PUBLIC BUILD_LISTENERS BUILD_MQTT MQTT_HOST="${MQTT_HOST}" MQTT_PORT="${MQTT_PORT}")
endif()

if(BUILD_TESTING)
    add_subdirectory(tests)
endif()

set(CPACK_PROJECT_NAME CoCo)
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)
