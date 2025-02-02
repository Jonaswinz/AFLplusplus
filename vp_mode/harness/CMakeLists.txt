cmake_minimum_required(VERSION 3.20)

project(test_client)

set(SOURCES
    test_client.cpp
)

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    set(CMAKE_PREFIX_PATH "${CMAKE_CURRENT_SOURCE_DIR}/easy_profiler_core/lib/cmake/easy_profiler;${CMAKE_PREFIX_PATH}")
    find_package(easy_profiler REQUIRED)
    target_link_libraries(test_client easy_profiler rt) 
    target_compile_definitions(test_client PRIVATE PROFILER_ENABLED)
endif()

add_executable(test_client ${SOURCES})