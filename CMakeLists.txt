cmake_minimum_required(VERSION 3.5.0)
project(CoCo VERSION 0.2.0 LANGUAGES CXX)
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -fsanitize=undefined")

include(CTest)
enable_testing()

find_package(mongocxx REQUIRED)
find_package(bsoncxx REQUIRED)

set(MULTIPLE_EXECUTORS ON CACHE BOOL "Enable multiple executors" FORCE)
set(BUILD_LISTENERS ON CACHE BOOL "Builds CoCo listeners" FORCE)
set(COMPUTE_NAMES ON CACHE BOOL "Compute CoCo names" FORCE)

set(COCO_NAME "CoCo" CACHE STRING "The CoCo Application Name")

set(CLIPS_INCLUDE_DIR /usr/local/include/clips CACHE PATH "CLIPS include directory")
set(CLIPS_LIB_DIR /usr/local/lib CACHE PATH "CLIPS library directory")

set(MONGODB_HOST "localhost" CACHE STRING "The MongoDB Host")
set(MONGODB_PORT "27017" CACHE STRING "The MongoDB Port")

option(ENABLE_MQTT "Enable MQTT" OFF)
option(ENABLE_TRANSFORMER "Enable CoCo transformer" OFF)
option(ENABLE_API "CoCo API" OFF)
option(ENABLE_AUTH "Enable CoCo authentication" OFF)

message(STATUS "CoCo name: ${COCO_NAME}")
message(STATUS "MongoDB host: ${MONGODB_HOST}")
message(STATUS "MongoDB port: ${MONGODB_PORT}")

add_subdirectory(extern/plexa)

add_library(CoCo src/coco_core.cpp src/coco_db.cpp src/coco_type.cpp src/coco_property.cpp src/coco_item.cpp src/coco_rule.cpp src/coco_executor.cpp src/coco_api.cpp src/mongo/mongo_db.cpp $<$<BOOL:${ENABLE_API}>:${CMAKE_CURRENT_SOURCE_DIR}/src/coco_api.cpp;${CMAKE_CURRENT_SOURCE_DIR}/src/server/coco_server.cpp> $<$<BOOL:${ENABLE_MQTT}>:${CMAKE_CURRENT_SOURCE_DIR}/src/mqtt/mqtt_connection.cpp>)
target_include_directories(CoCo PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include> $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include/mongo> ${LIBMONGOCXX_INCLUDE_DIR} ${LIBBSONCXX_INCLUDE_DIR} ${CLIPS_INCLUDE_DIR} $<$<BOOL:${ENABLE_API}>:${CMAKE_CURRENT_SOURCE_DIR}/include/server> $<$<BOOL:${ENABLE_MQTT}>:${CMAKE_CURRENT_SOURCE_DIR}/include/mqtt>)
add_dependencies(CoCo PlExA)
target_link_directories(CoCo PUBLIC ${CLIPS_LIB_DIR})
target_link_libraries(CoCo PUBLIC PlExA clips mongo::bsoncxx_shared mongo::mongocxx_shared)
target_compile_definitions(CoCo PUBLIC COCO_NAME="${COCO_NAME}" MONGODB_HOST="${MONGODB_HOST}" MONGODB_PORT="${MONGODB_PORT}")

if(ENABLE_API OR ENABLE_TRANSFORMER)
    add_subdirectory(extern/rationet)

    add_dependencies(CoCo ratioNet)
    target_link_libraries(CoCo PUBLIC ratioNet)
endif()

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

message(STATUS "CoCo API: ${ENABLE_API}")
if(ENABLE_API)
    set(CLIENT_DIR ${CMAKE_CURRENT_SOURCE_DIR}/coco-client CACHE STRING "Client directory")
    message(STATUS "Client directory: ${CLIENT_DIR}")

    target_compile_definitions(CoCo PUBLIC ENABLE_API CLIENT_DIR="${CLIENT_DIR}")
endif()

message(STATUS "CoCo transformer: ${ENABLE_TRANSFORMER}")
if(ENABLE_TRANSFORMER)
    set(TRANSFORMER_HOST "localhost" CACHE STRING "The transformer Host")
    set(TRANSFORMER_PORT "5005" CACHE STRING "The transformer Port")

    message(STATUS "transformer host: ${TRANSFORMER_HOST}")
    message(STATUS "transformer port: ${TRANSFORMER_PORT}")

    target_compile_definitions(CoCo PUBLIC ENABLE_TRANSFORMER TRANSFORMER_HOST="${TRANSFORMER_HOST}" TRANSFORMER_PORT=${TRANSFORMER_PORT})
endif()

message(STATUS "CoCo authentication: ${ENABLE_AUTH}")
if(ENABLE_AUTH)
    set(MONGODB_USERS_HOST "localhost" CACHE STRING "The MongoDB Users Host")
    set(MONGODB_USERS_PORT "27017" CACHE STRING "The MongoDB Users Port")

    message(STATUS "MongoDB users host: ${MONGODB_USERS_HOST}")
    message(STATUS "MongoDB users port: ${MONGODB_USERS_PORT}")

    target_compile_definitions(CoCo PUBLIC ENABLE_AUTH MONGODB_USERS_HOST="${MONGODB_USERS_HOST}" MONGODB_USERS_PORT="${MONGODB_USERS_PORT}")
endif()

if(MSVC)
    target_compile_options(CoCo PRIVATE /W4)
else()
    target_compile_options(CoCo PRIVATE -Wall -Wextra -Wpedantic)
    if (ENABLE_COVERAGE)
        if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
            target_compile_options(CoCo PRIVATE --coverage)
            target_link_libraries(CoCo PUBLIC gcov)
        endif()
    endif()
endif()

#if(BUILD_TESTING)
#    add_subdirectory(tests)
#endif()

set(CPACK_PROJECT_NAME CoCo)
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)