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

include(CTest)
enable_testing()

set(CLIPS_INCLUDE_DIR /usr/local/include/clips CACHE PATH "CLIPS include directory")
set(CLIPS_LIB_DIR /usr/local/lib CACHE PATH "CLIPS library directory")

set(COCO_NAME "CoCo" CACHE STRING "The CoCo Application Name")
set(MQTT ON CACHE BOOL "Enables MQTT")
set(MONGO_DB ON CACHE BOOL "Enables MongoDB")

file(GLOB COCO_SOURCES src/*.cpp)
file(GLOB COCO_HEADERS include/*.h)

if(MQTT)
    find_package(PahoMqttCpp REQUIRED)

    set(MQTT_HOST "tcp://127.0.0.1" CACHE STRING "The MQTT Host")
    set(MQTT_PORT "1883" CACHE STRING "The MQTT Port")
    set(MQTT_AUTH OFF CACHE BOOL "Enables MQTT authentication")
    set(MQTT_USERNAME "" CACHE STRING "The MQTT Username")
    set(MQTT_PASSWORD "" CACHE STRING "The MQTT Password")

    message(STATUS "MQTT host:              ${MQTT_HOST}")
    message(STATUS "MQTT port:              ${MQTT_PORT}")
    if(NOT MQTT_USERNAME STREQUAL "" OR NOT MQTT_PASSWORD STREQUAL "")
        set(MQTT_AUTH ON CACHE BOOL "Enables app MQTT authentication" FORCE)
    endif()

    list(APPEND COCO_SOURCES src/mqtt/mqtt_middleware.cpp)
endif()

if(MONGO_DB)
    find_package(mongocxx REQUIRED)
    find_package(bsoncxx REQUIRED)

    set(MONGODB_HOST "localhost" CACHE STRING "The MongoDB Host")
    set(MONGODB_PORT "27017" CACHE STRING "The MongoDB Port")

    message(STATUS "MongoDB host:           ${MONGODB_HOST}")
    message(STATUS "MongoDB port:           ${MONGODB_PORT}")

    list(APPEND COCO_SOURCES src/mongo/mongo_db.cpp)
endif()

option(SOLVING_MONITORING "Publishes any changes during the resolution process" OFF)

set(MULTIPLE_EXECUTORS ON CACHE BOOL "Allows different executors" FORCE)
set(COMPUTE_NAMES ON CACHE BOOL "Computes the objects' names" FORCE)

message(STATUS "CoCo app name:          ${COCO_NAME}")

add_subdirectory(extern/plexa)

add_library(${PROJECT_NAME} STATIC ${COCO_SOURCES})
GENERATE_EXPORT_HEADER(${PROJECT_NAME})
target_include_directories(${PROJECT_NAME} PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>/include $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}> $<INSTALL_INTERFACE:> ${CLIPS_INCLUDE_DIR} ${LIBMONGOCXX_INCLUDE_DIR} ${LIBBSONCXX_INCLUDE_DIR})
target_link_directories(${PROJECT_NAME} PUBLIC ${CLIPS_LIB_DIR})
target_link_libraries(${PROJECT_NAME} PUBLIC PlExA clips)
target_compile_definitions(${PROJECT_NAME} PUBLIC COCO_NAME="${COCO_NAME}")

if(MQTT)
    target_link_libraries(${PROJECT_NAME} PUBLIC PahoMqttCpp::paho-mqttpp3-static)
    target_compile_definitions(${PROJECT_NAME} PUBLIC MQTT_HOST="${MQTT_HOST}" MQTT_PORT="${MQTT_PORT}")
    if(MQTT_AUTH)
        target_compile_definitions(${PROJECT_NAME} PUBLIC MQTT_AUTH MQTT_USERNAME="${MQTT_USERNAME}" MQTT_PASSWORD="${MQTT_PASSWORD}")
    endif()
endif()

if(MONGO_DB)
    target_link_libraries(${PROJECT_NAME} PUBLIC mongo::bsoncxx_shared mongo::mongocxx_shared)
    target_compile_definitions(${PROJECT_NAME} PUBLIC MONGODB_HOST="${MONGODB_HOST}" MONGODB_PORT="${MONGODB_PORT}")
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