cmake_minimum_required(VERSION 3.21)
project(CoCo VERSION 0.2.0 LANGUAGES CXX)

include(CTest)
enable_testing()

set(COCO_NAME "CoCo" CACHE STRING "The CoCo Application Name")

set(CLIPS_INCLUDE_DIR /usr/local/include/clips CACHE PATH "CLIPS include directory")
set(CLIPS_LIB_DIR /usr/local/lib CACHE PATH "CLIPS library directory")

option(BUILD_MONGODB "Build MongoDB drivers" OFF)
option(BUILD_COCO_SERVER "Build the CoCo server" OFF)
option(BUILD_MQTT "Build MQTT middleware" OFF)
option(BUILD_DELIBERATIVE "Build CoCo deliberative" OFF)
option(BUILD_LLM "Build CoCo LLM" OFF)
option(BUILD_AUTH "Build CoCo authentication" OFF)

if(BUILD_DELIBERATIVE OR BUILD_LLM OR BUILD_AUTH)
    set(BUILD_MONGODB ON CACHE BOOL "Build MongoDB drivers" FORCE)
endif()

if(BUILD_AUTH)
    set(BUILD_COCO_SERVER ON CACHE BOOL "Build the CoCo server" FORCE)
endif()

message(STATUS "CoCo name: ${COCO_NAME}")
message(STATUS "Build MongoDB drivers: ${BUILD_MONGODB}")
message(STATUS "Build CoCo server: ${BUILD_COCO_SERVER}")
message(STATUS "Build MQTT middleware: ${BUILD_MQTT}")
message(STATUS "Build CoCo deliberative: ${BUILD_DELIBERATIVE}")
message(STATUS "Build CoCo LLM: ${BUILD_LLM}")
message(STATUS "Build CoCo authentication: ${BUILD_AUTH}")

add_library(CoCo src/coco.cpp src/coco_module.cpp src/coco_property.cpp src/coco_type.cpp src/coco_item.cpp src/coco_db.cpp)
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

    if(BUILD_AUTH)
        set(BUILD_CRYPTO ON CACHE BOOL "Build the crypto library" FORCE)

        set(MONGODB_USERS_HOST "localhost" CACHE STRING "The MongoDB Users Host")
        set(MONGODB_USERS_PORT "27018" CACHE STRING "The MongoDB Users Port")

        message(STATUS "MongoDB users host: ${MONGODB_USERS_HOST}")
        message(STATUS "MongoDB users port: ${MONGODB_USERS_PORT}")

        target_compile_definitions(CoCo PUBLIC MONGODB_USERS_HOST="${MONGODB_USERS_HOST}" MONGODB_USERS_PORT="${MONGODB_USERS_PORT}")
    endif()
endif()

if(BUILD_COCO_SERVER)
    if(NOT TARGET ratioNet)
        add_subdirectory(extern/rationet)
    endif()

    set(CLIENT_DIR ${CMAKE_CURRENT_SOURCE_DIR}/gui/app CACHE STRING "Client directory")
    message(STATUS "Client directory: ${CLIENT_DIR}")

    target_sources(CoCo PRIVATE src/server/coco_server.cpp)
    target_include_directories(CoCo PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include/server> $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include/auth>)
    add_dependencies(CoCo ratioNet)
    target_link_libraries(CoCo PUBLIC ratioNet)
    target_compile_definitions(CoCo PUBLIC BUILD_LISTENERS BUILD_SERVER CLIENT_DIR="${CLIENT_DIR}")

    if(BUILD_AUTH)
        set(ENABLE_SSL ON CACHE BOOL "Enable SSL" FORCE)

        target_sources(CoCo PRIVATE src/auth/coco_auth.cpp src/auth/auth_db.cpp)
    else()
        target_sources(CoCo PRIVATE src/auth/coco_noauth.cpp)
    endif()

    add_custom_target(npm_lib_install ALL COMMAND npm install WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/gui/lib COMMENT "Installing the TypeScript library" VERBATIM)
    add_custom_target(npm_lib_build ALL COMMAND npm run build WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/gui/lib COMMENT "Building the TypeScript library" VERBATIM DEPENDS npm_lib_install)
    add_custom_target(npm_gui_install ALL COMMAND npm install WORKING_DIRECTORY ${CLIENT_DIR} COMMENT "Installing the TypeScript application" VERBATIM DEPENDS npm_lib_build)
    add_custom_target(npm_gui_build ALL COMMAND npm run build WORKING_DIRECTORY ${CLIENT_DIR} COMMENT "Building the TypeScript application" VERBATIM DEPENDS npm_gui_install)
endif()

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

if(BUILD_DELIBERATIVE)
    add_subdirectory(extern/plexa)

    target_include_directories(CoCo PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include/deliberative>)
    target_sources(CoCo PRIVATE src/deliberative/coco_deliberative.cpp src/deliberative/coco_executor.cpp src/deliberative/deliberative_db.cpp)

    if (BUILD_COCO_SERVER)
        target_sources(CoCo PRIVATE src/deliberative/deliberative_server.cpp)
    endif()

    add_dependencies(CoCo PlExA)
    target_link_libraries(CoCo PUBLIC PlExA)
endif()

if(BUILD_LLM)
    if(NOT TARGET ratioNet)
        add_subdirectory(extern/rationet)
    endif()

    set(LLM_HOST "localhost" CACHE STRING "The LLM Host")
    set(LLM_PORT "5000" CACHE STRING "The LLM Port")

    message(STATUS "LLM host: ${LLM_HOST}")
    message(STATUS "LLM port: ${LLM_PORT}")

    target_include_directories(CoCo PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include/llm>)
    target_sources(CoCo PRIVATE src/llm/coco_llm.cpp src/llm/llm_db.cpp)
    add_dependencies(CoCo ratioNet)
    target_link_libraries(CoCo PUBLIC ratioNet)
    target_compile_definitions(CoCo PUBLIC BUILD_LISTENERS BUILD_LLM LLM_HOST="${LLM_HOST}" LLM_PORT=${LLM_PORT})
endif()

if(BUILD_TESTING)
    add_subdirectory(tests)
endif()

set(CPACK_PROJECT_NAME CoCo)
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)
