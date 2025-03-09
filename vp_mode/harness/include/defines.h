#ifndef DEFINES_H
#define DEFINES_H

#include <iostream>
#include <cstdlib>
#include <mqueue.h>
#include <cerrno>
#include <string.h>
#include <algorithm>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/stat.h>
#include <fstream>
#include <cstdarg>
#include <csignal>
#include <cstring>
#include <sys/wait.h>
#include <unistd.h>
#include <sstream>
#include <ctime>
#include <vector>
#include <future>
#include <mutex>

// Import vp-testing-interface client.
#include "testing_client.h"

// Import profiler if used (can be set here or with INCLUDE_VP_HARNESS_PROFILER option).
#ifdef PROFILER_ENABLED
    #include <easy/profiler.h>
#else
    #define EASY_FUNCTION(...)
    #define EASY_BLOCK(...)
    #define EASY_END_BLOCK 
    #define EASY_PROFILER_ENABLE 
#endif

// Settings
#define PROFILING_COUNT 15
#define AFL_MODE
#define OWN_NAME "test_client"
#define MAX_VP_INSTANCES 20
// End Settings

// Data to enable shared memory fuzzing for AFLplusplus
#define FS_OPT_ENABLED 0x80000001
#define FS_OPT_SHDMEM_FUZZ 0x01000000

// Logging macro
#define LOG_MESSAGE(type, format, ...) logger::log(type, format, ##__VA_ARGS__)

#endif