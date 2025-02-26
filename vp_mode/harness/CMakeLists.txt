cmake_minimum_required(VERSION 3.20)

project(test_client)

add_executable(test_client test_client.cpp)

# Getting vp-testing-interface
include(FetchContent)

FetchContent_Declare(
  vp_testing_interface
  GIT_REPOSITORY https://github.com/Jonaswinz/vp-testing-interface
  GIT_TAG        master
)
FetchContent_MakeAvailable(vp_testing_interface)

option(INCLUDE_VP_HARNESS_PROFILER "Enable profiling of the vp harness with easy_profiler" OFF)

if(INCLUDE_VP_HARNESS_PROFILER)
    set(CMAKE_PREFIX_PATH "${CMAKE_CURRENT_SOURCE_DIR}/easy_profiler_core/lib/cmake/easy_profiler;${CMAKE_PREFIX_PATH}")
    find_package(easy_profiler REQUIRED)
    target_link_libraries(test_client easy_profiler) 
    target_compile_definitions(test_client PRIVATE PROFILER_ENABLED)
endif()

target_link_libraries(test_client rt vp-testing-interface) 