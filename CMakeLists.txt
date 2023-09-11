cmake_minimum_required(VERSION 3.0.0)
project(COCO VERSION 0.1.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -fsanitize=undefined")

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

include(GenerateExportHeader)
include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

find_package(Threads REQUIRED)
find_package(Boost REQUIRED)
find_package(PahoMqttCpp REQUIRED)
find_package(mongocxx REQUIRED)
find_package(bsoncxx REQUIRED)

include(CTest)
enable_testing()

set(CLIPS_INCLUDE_DIR /usr/local/include/clips CACHE PATH "CLIPS include directory")
set(CLIPS_LIB_DIR /usr/local/lib CACHE PATH "CLIPS library directory")

set(COCO_APP "COCO" CACHE STRING "The COCO App")
set(COCO_INSTANCE "COCO" CACHE STRING "The COCO Instance")
set(COCO_RULES "" CACHE STRING "The COCO Rules")
set(MQTT_APP_HOST "tcp://127.0.0.1" CACHE STRING "The MQTT Host")
set(MQTT_APP_PORT "1883" CACHE STRING "The MQTT Port")
set(MQTT_INSTANCE_HOST "tcp://127.0.0.1" CACHE STRING "The MQTT Host")
set(MQTT_INSTANCE_PORT "1883" CACHE STRING "The MQTT Port")
set(MQTT_APP_AUTH OFF CACHE BOOL "Enables MQTT authentication")
set(MQTT_APP_USERNAME "" CACHE STRING "The MQTT Username")
set(MQTT_APP_PASSWORD "" CACHE STRING "The MQTT Password")
set(MQTT_INSTANCE_AUTH OFF CACHE BOOL "Enables MQTT authentication")
set(MQTT_INSTANCE_USERNAME "" CACHE STRING "The MQTT Username")
set(MQTT_INSTANCE_PASSWORD "" CACHE STRING "The MQTT Password")
set(MONGODB_APP_HOST "localhost" CACHE STRING "The MongoDB Host")
set(MONGODB_APP_PORT "27017" CACHE STRING "The MongoDB Port")
set(MONGODB_INSTANCE_HOST "localhost" CACHE STRING "The MongoDB Host")
set(MONGODB_INSTANCE_PORT "27017" CACHE STRING "The MongoDB Port")

option(SOLVING_MONITORING "Publishes any changes during the resolution process" OFF)

set(MULTIPLE_EXECUTORS ON CACHE BOOL "Allows different executors" FORCE)
set(COMPUTE_NAMES ON CACHE BOOL "Computes the objects' names" FORCE)

message(STATUS "CoCo app:               ${COCO_APP}")
message(STATUS "CoCo instance:          ${COCO_INSTANCE}")
message(STATUS "CoCo rules:             ${COCO_RULES}")

message(STATUS "MQTT app host:          ${MQTT_APP_HOST}")
message(STATUS "MQTT app port:          ${MQTT_APP_PORT}")
if(NOT MQTT_APP_USERNAME STREQUAL "" OR NOT MQTT_APP_PASSWORD STREQUAL "")
    set(MQTT_APP_AUTH ON CACHE BOOL "Enables app MQTT authentication" FORCE)
endif()
message(STATUS "MQTT app auth:          ${MQTT_APP_AUTH}")
if(MQTT_APP_AUTH)
    message(STATUS "MQTT app username:      ${MQTT_APP_USERNAME}")
    message(STATUS "MQTT app password:      ${MQTT_APP_PASSWORD}")
endif()
message(STATUS "MongoDB app host:       ${MONGODB_HOST}")
message(STATUS "MongoDB app port:       ${MONGODB_PORT}")

message(STATUS "MQTT instance host:     ${MQTT_INSTANCE_HOST}")
message(STATUS "MQTT instance port:     ${MQTT_INSTANCE_PORT}")
if(NOT MQTT_INSTANCE_USERNAME STREQUAL "" OR NOT MQTT_INSTANCE_PASSWORD STREQUAL "")
    set(MQTT_INSTANCE_AUTH ON CACHE BOOL "Enables instance MQTT authentication" FORCE)
endif()
message(STATUS "MQTT instance auth:     ${MQTT_INSTANCE_AUTH}")
if(MQTT_INSTANCE_AUTH)
    message(STATUS "MQTT instance username: ${MQTT_INSTANCE_USERNAME}")
    message(STATUS "MQTT instance password: ${MQTT_INSTANCE_PASSWORD}")
endif()
message(STATUS "MongoDB instance host: ${MONGODB_INSTANCE_HOST}")
message(STATUS "MongoDB instance port: ${MONGODB_INSTANCE_PORT}")

add_subdirectory(extern/plexa)

file(GLOB COCO_SOURCES src/*.cpp)
file(GLOB COCO_HEADERS include/*.h)

add_library(${PROJECT_NAME} STATIC ${COCO_SOURCES})
GENERATE_EXPORT_HEADER(${PROJECT_NAME})
target_include_directories(${PROJECT_NAME} PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>/include $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}> $<INSTALL_INTERFACE:> ${CLIPS_INCLUDE_DIR} ${LIBMONGOCXX_INCLUDE_DIR} ${LIBBSONCXX_INCLUDE_DIR})
target_link_directories(${PROJECT_NAME} PUBLIC ${CLIPS_LIB_DIR})
target_link_libraries(${PROJECT_NAME} PUBLIC PlExA PahoMqttCpp::paho-mqttpp3-static clips mongo::bsoncxx_shared mongo::mongocxx_shared)
target_compile_definitions(${PROJECT_NAME} PUBLIC COCO_APP="${COCO_APP}" COCO_INSTANCE="${COCO_INSTANCE}" COCO_RULES="${COCO_RULES}" MQTT_APP_HOST="${MQTT_APP_HOST}" MQTT_APP_PORT="${MQTT_APP_PORT}" MQTT_INSTANCE_HOST="${MQTT_INSTANCE_HOST}" MQTT_INSTANCE_PORT="${MQTT_INSTANCE_PORT}" MONGODB_APP_HOST="${MONGODB_APP_HOST}" MONGODB_APP_PORT="${MONGODB_APP_PORT}" MONGODB_INSTANCE_HOST="${MONGODB_INSTANCE_HOST}" MONGODB_INSTANCE_PORT="${MONGODB_INSTANCE_PORT}")
if(MQTT_APP_AUTH)
    target_compile_definitions(${PROJECT_NAME} PUBLIC MQTT_AUTH MQTT_USERNAME="${MQTT_USERNAME}" MQTT_PASSWORD="${MQTT_PASSWORD}")
endif()
if(MQTT_INSTANCE_AUTH)
    target_compile_definitions(${PROJECT_NAME} PUBLIC MQTT_AUTH MQTT_USERNAME="${MQTT_USERNAME}" MQTT_PASSWORD="${MQTT_PASSWORD}")
endif()

configure_file(rules/rules.clp rules/rules.clp COPYONLY)

message(STATUS "Solving monitoring:     ${SOLVING_MONITORING}")
if(SOLVING_MONITORING)
    target_compile_definitions(${PROJECT_NAME} PRIVATE SOLVING_MONITORING)
endif()

if(BUILD_TESTING)
    add_subdirectory(tests)
endif()

if(MSVC)
    target_compile_options(${PROJECT_NAME} PRIVATE /W4)
else()
    target_compile_options(${PROJECT_NAME} PRIVATE -Wall -Wextra -pedantic)
endif()

install(
    TARGETS ${PROJECT_NAME}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
)
install(
    FILES ${COCO_HEADERS} ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}_export.h
    INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/${PROJECT_NAME}
)
install(
    FILES rules/rules.clp
    DESTINATION ${CMAKE_INSTALL_LIBDIR}
)
configure_package_config_file(src/${PROJECT_NAME}Config.cmake.in ${PROJECT_NAME}Config.cmake INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/${PROJECT_NAME} PATH_VARS CMAKE_INSTALL_INCLUDEDIR)
write_basic_package_version_file(${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}ConfigVersion.cmake VERSION 1.0.0 COMPATIBILITY SameMajorVersion)
install(
    FILES ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}Config.cmake ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}ConfigVersion.cmake
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/${PROJECT_NAME}
)

set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)