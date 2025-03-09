cmake_minimum_required(VERSION 3.20)

project(harness)

set(src "${CMAKE_CURRENT_SOURCE_DIR}/src")
set(inc "${CMAKE_CURRENT_SOURCE_DIR}/include")

include_directories(${inc})

add_executable(harness
    ${src}/logger.cpp
    ${src}/vp_client.cpp
    ${src}/afl_client.cpp
    ${src}/main.cpp
)

# Getting vp-testing-interface
include(FetchContent)

FetchContent_Declare(
  vp_testing_interface
  GIT_REPOSITORY https://github.com/Jonaswinz/vp-testing-interface
  GIT_TAG        master
)
FetchContent_MakeAvailable(vp_testing_interface)

# Option to enable profiling
option(INCLUDE_VP_HARNESS_PROFILER "Enable profiling of the VP harness with easy_profiler" OFF)

if(INCLUDE_VP_HARNESS_PROFILER)
    # Define the URL of the latest release asset
    set(EASY_PROFILER_VERSION "v2.1.0")
    set(EASY_PROFILER_URL "https://github.com/yse/easy_profiler/releases/download/${EASY_PROFILER_VERSION}/easy_profiler_core-${EASY_PROFILER_VERSION}-linux-x64-libc-2.27.tar.gz")

    # Define the destination directory for easy_profiler
    set(EASY_PROFILER_DIR "${CMAKE_BINARY_DIR}/easy_profiler")
    set(EASY_PROFILER_ARCHIVE "${CMAKE_BINARY_DIR}/easy_profiler.tar.gz")

    # Download and extract the release asset
    file(DOWNLOAD ${EASY_PROFILER_URL} ${EASY_PROFILER_ARCHIVE} SHOW_PROGRESS)
    file(MAKE_DIRECTORY ${EASY_PROFILER_DIR})
    execute_process(COMMAND ${CMAKE_COMMAND} -E tar xzf ${EASY_PROFILER_ARCHIVE} WORKING_DIRECTORY ${EASY_PROFILER_DIR})

    file(COPY ${EASY_PROFILER_DIR}/bin/libeasy_profiler.so DESTINATION ${EASY_PROFILER_DIR}/lib)

    # Add include directory for eas4y_profiler
    target_include_directories(harness PRIVATE ${EASY_PROFILER_DIR}/include)

    # Add library paths and explicitly link the .so file
    target_link_directories(harness PRIVATE ${EASY_PROFILER_DIR}/bin)
    target_link_libraries(harness PRIVATE ${EASY_PROFILER_LIB})

    # Define compile-time flag
    target_compile_definitions(harness PRIVATE PROFILER_ENABLED)
endif()

# Link test_client with required libraries
target_link_libraries(harness rt vp-testing-interface) 

# After building the executable, make a copy with name "test_client" for compatibility with the old name.
add_custom_command(TARGET harness POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:harness> ./test_client
)