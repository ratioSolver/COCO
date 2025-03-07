cmake_minimum_required(VERSION 3.21)
project(CoCo VERSION 0.2.0 LANGUAGES CXX)

include(CTest)
enable_testing()

set(CLIPS_INCLUDE_DIR /usr/local/include/clips CACHE PATH "CLIPS include directory")
set(CLIPS_LIB_DIR /usr/local/lib CACHE PATH "CLIPS library directory")

find_package(mongocxx REQUIRED)
find_package(bsoncxx REQUIRED)

option(ENABLE_MQTT "Enable MQTT" OFF)

add_library(CoCo src/coco.cpp src/coco_db.cpp)
target_compile_features(CoCo PUBLIC cxx_std_17)
target_include_directories(CoCo PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include> ${CLIPS_INCLUDE_DIR} ${LIBMONGOCXX_INCLUDE_DIR} ${LIBBSONCXX_INCLUDE_DIR})
if(NOT TARGET ratioNet)
    add_subdirectory(extern/rationet)
endif()
add_dependencies(CoCo ratioNet)
target_link_directories(CoCo PUBLIC ${CLIPS_LIB_DIR})
target_link_libraries(CoCo PUBLIC ratioNet clips mongo::bsoncxx_shared mongo::mongocxx_shared)
setup_sanitizers(CoCo)

message(STATUS "Enable MQTT: ${ENABLE_MQTT}")
if(ENABLE_MQTT)
    find_package(PahoMqttCpp REQUIRED)

    set(MQTT_HOST "localhost" CACHE STRING "The MQTT Host")
    set(MQTT_PORT "1883" CACHE STRING "The MQTT Port")

    message(STATUS "MQTT host: ${MQTT_HOST}")
    message(STATUS "MQTT port: ${MQTT_PORT}")

    target_include_directories(CoCo PUBLIC ${PAHO_MQTT_CPP_INCLUDE_DIR})
    target_link_libraries(CoCo PUBLIC ${PAHO_MQTT_CPP_LIBRARIES})
    target_compile_definitions(CoCo PUBLIC ENABLE_MQTT MQTT_HOST="${MQTT_HOST}" MQTT_PORT="${MQTT_PORT}")
endif()

if(BUILD_TESTING)
    add_subdirectory(tests)
endif()

set(CPACK_PROJECT_NAME CoCo)
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)
