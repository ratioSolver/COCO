cmake_minimum_required(VERSION 3.5.0)
project(CoCo VERSION 0.2.0 LANGUAGES CXX)
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -fsanitize=undefined")

include(CTest)
enable_testing()

add_subdirectory(extern/plexa)

add_library(CoCo src/coco_core.cpp src/coco_db.cpp src/sensor_type.cpp src/sensor.cpp)
add_dependencies(CoCo oRatioLib)
target_link_libraries(CoCo PUBLIC oRatioLib)
target_include_directories(CoCo PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>)

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

set(CPACK_PROJECT_NAME CoCo)
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)
