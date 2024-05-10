cmake_minimum_required(VERSION 3.5.0)
project(CoCo VERSION 0.2.0 LANGUAGES CXX)
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -fsanitize=undefined")

include(CTest)
enable_testing()

find_package(mongocxx REQUIRED)
find_package(bsoncxx REQUIRED)

set(MULTIPLE_EXECUTORS ON CACHE BOOL "Enable multiple executors" FORCE)

set(COCO_NAME "CoCo" CACHE STRING "The CoCo Application Name")

set(CLIPS_INCLUDE_DIR /usr/local/include/clips CACHE PATH "CLIPS include directory")
set(CLIPS_LIB_DIR /usr/local/lib CACHE PATH "CLIPS library directory")

set(MONGODB_HOST "localhost" CACHE STRING "The MongoDB Host")
set(MONGODB_PORT "27017" CACHE STRING "The MongoDB Port")

message(STATUS "CoCo name: ${COCO_NAME}")
message(STATUS "MongoDB host: ${MONGODB_HOST}")
message(STATUS "MongoDB port: ${MONGODB_PORT}")

add_subdirectory(extern/plexa)

add_library(CoCo src/coco_core.cpp src/coco_db.cpp src/coco_type.cpp src/coco_item.cpp src/coco_executor.cpp src/mongo/mongo_db.cpp)
target_include_directories(CoCo PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include> $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include/mongo> ${LIBMONGOCXX_INCLUDE_DIR} ${LIBBSONCXX_INCLUDE_DIR} ${CLIPS_INCLUDE_DIR})
add_dependencies(CoCo PlExA)
target_link_directories(CoCo PUBLIC ${CLIPS_LIB_DIR})
target_link_libraries(CoCo PUBLIC PlExA clips mongo::bsoncxx_shared mongo::mongocxx_shared)
target_compile_definitions(CoCo PUBLIC COCO_NAME="${COCO_NAME}" MONGODB_HOST="${MONGODB_HOST}" MONGODB_PORT="${MONGODB_PORT}")

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
